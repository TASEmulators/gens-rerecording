#include "luascript.h"
#include "gens.h"
#include "save.h"
#include "g_main.h"
#include "guidraw.h"
#include "movie.h"
#include "vdp_io.h"
#include "drawutil.h"
#include <assert.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

// the emulator must provide these so that we can implement
// the various functions the user can call from their lua script
// (this interface with the emulator needs cleanup, I know)
extern int (*Update_Frame)();
extern int (*Update_Frame_Fast)();
extern "C" unsigned int ReadValueAtHardwareAddress(unsigned int address, unsigned int size);
extern "C" void WriteValueAtHardwareAdress(unsigned int address, unsigned int value, unsigned int size);
extern "C" int disableSound2, disableRamSearchUpdate;
extern "C" int Clear_Sound_Buffer(void);
extern long long GetCurrentInputCondensed();
extern void SetNextInputCondensed(long long input, long long mask);
extern int Set_Current_State(int Num);
extern int Update_Emulation_One(HWND hWnd);
extern void Update_Emulation_One_Before(HWND hWnd);
extern void Update_Emulation_After_Fast(HWND hWnd);
extern void Update_Emulation_One_Before_Minimal();
extern int Update_Frame_Adjusted();
extern int Update_Frame_Hook();
extern int Update_Frame_Fast_Hook();
extern void Update_Emulation_After_Controlled(HWND hWnd, bool flip);
extern void UpdateLagCount();
extern bool BackgroundInput;
extern bool Step_Gens_MainLoop(bool allowSleep, bool allowEmulate);
extern bool frameadvSkipLagForceDisable;
extern "C" void Put_Info_NonImmediate(char *Message, int Duration);
extern int Show_Genesis_Screen();
extern const char* GensPlayMovie(const char* filename, bool silent);
extern void GensReplayMovie();

extern "C" {
	#include "lua/src/lua.h"
	#include "lua/src/lauxlib.h"
	#include "lua/src/lualib.h"
	#include "lua/src/lstate.h"
};

enum SpeedMode
{
	SPEEDMODE_NORMAL,
	SPEEDMODE_NOTHROTTLE,
	SPEEDMODE_TURBO,
	SPEEDMODE_MAXIMUM,
};

struct LuaContextInfo {
	lua_State* L; // the Lua state
	bool started; // script has been started and hasn't yet been terminated, although it may not be currently running
	bool running; // script is currently running code (either the main call to the script or the callbacks it registered)
	bool returned; // main call to the script has returned (but it may still be active if it registered callbacks)
	bool crashed; // true if script has errored out
	bool restart; // if true, tells the script-running code to restart the script when the script stops
	bool restartLater; // set to true when a still-running script is stopped so that RestartAllLuaScripts can know which scripts to restart
	int worryCount; // counts up as the script executes, gets reset when the application is able to process messages, triggers a warning prompt if it gets too high
	bool panic; // if set to true, tells the script to terminate as soon as it can do so safely (used because directly calling lua_close() or luaL_error() is unsafe in some contexts)
	bool ranExit; // used to prevent a registered exit callback from ever getting called more than once
	bool guiFuncsNeedDeferring; // true whenever GUI drawing would be cleared by the next emulation update before it would be visible, and thus needs to be deferred until after the next emulation update
	int numDeferredGUIFuncs; // number of deferred function calls accumulated, used to impose an arbitrary limit to avoid running out of memory
	bool ranFrameAdvance; // false if gens.frameadvance() hasn't been called yet
	int transparencyModifier; // values less than 255 will scale down the opacity of whatever the GUI renders, values greater than 255 will increase the opacity of anything transparent the GUI renders
	SpeedMode speedMode; // determines how gens.frameadvance() acts
	char panicMessage [64]; // a message to print if the script terminates due to panic being set
	std::string lastFilename; // path to where the script last ran from so that restart can work (note: storing the script in memory instead would not be useful because we always want the most up-to-date script from file)
	// callbacks into the lua window... these don't need to exist per context the way I'm using them, but whatever
	void(*print)(int uid, const char* str);
	void(*onstart)(int uid);
	void(*onstop)(int uid, bool statusOK);
};
std::map<int, LuaContextInfo*> luaContextInfo;
std::map<lua_State*, int> luaStateToUIDMap;
int g_numScriptsStarted = 0;
bool g_stopAllScriptsEnabled = true;

#define USE_INFO_STACK
#ifdef USE_INFO_STACK
	std::vector<LuaContextInfo*> infoStack;
	#define GetCurrentInfo() *infoStack.front() // should be faster but relies on infoStack correctly being updated to always have the current info in the first element
#else
	std::map<lua_State*, LuaContextInfo*> luaStateToContextMap;
	#define GetCurrentInfo() *luaStateToContextMap[L] // should always work but might be slower
#endif


static const char* luaCallIDStrings [] =
{
	"CALL_BEFOREEMULATION",
	"CALL_AFTEREMULATION",
	"CALL_AFTEREMULATIONGUI",
	"CALL_BEFOREEXIT",
	"CALL_BEFORESAVE",
	"CALL_AFTERLOAD",
};

static const int _makeSureWeHaveTheRightNumberOfStrings [sizeof(luaCallIDStrings)/sizeof(*luaCallIDStrings) == LUACALL_COUNT ? 1 : 0];

void StopScriptIfFinished(int uid, bool justReturned = false);

int add_memory_proc (int *list, int addr, int &numprocs)
{
	if (numprocs >= 16) return 1;
	int i = 0;
	while ((i < numprocs) && (list[i] < addr))
		i++;
	for (int j = numprocs; j >= i; j--)
		list[j] = list[j-i];
	list[i] = addr;
	numprocs++;
	return 0;
}
int del_memory_proc (int *list, int addr, int &numprocs)
{
	if (numprocs <= 0) return 1;
	int i = 0;
//	bool found = false;
	while ((i < numprocs) && (list[i] != addr))
		i++;
	if (list[i] == addr)
	{
		while (i < numprocs)
		{
			list[i] = list[i+1];
			i++;
		}
		list[i]=0;
		return 0;
	}
	else return 1;
}
#define memreg(proc)\
int proc##_writelist[16];\
int proc##_readlist[16];\
int proc##_execlist[16];\
int proc##_numwritefuncs = 0;\
int proc##_numreadfuncs = 0;\
int proc##_numexecfuncs = 0;\
int memory_register##proc##write (lua_State* L)\
{\
	unsigned int addr = luaL_checkinteger(L, 1);\
	char Name[16];\
	sprintf(Name,#proc"_W%08X",addr);\
	if (lua_type(L,2) != LUA_TNIL && lua_type(L,2) != LUA_TFUNCTION)\
		luaL_error(L, "function or nil expected in arg 2 to memory.register" #proc "write");\
	lua_setfield(L, LUA_REGISTRYINDEX, Name);\
	if (lua_type(L,2) == LUA_TNIL) \
		del_memory_proc(proc##_writelist, addr, proc##_numwritefuncs);\
	else \
		if (add_memory_proc(proc##_writelist, addr, proc##_numwritefuncs)) \
			luaL_error(L, #proc "write hook registry is full.");\
	return 0;\
}\
int memory_register##proc##read (lua_State* L)\
{\
	unsigned int addr = luaL_checkinteger(L, 1);\
	char Name[16];\
	sprintf(Name,#proc"_R%08X",addr);\
	if (lua_type(L,2) != LUA_TNIL && lua_type(L,2) != LUA_TFUNCTION)\
		luaL_error(L, "function or nil expected in arg 2 to memory.register" #proc "write");\
	lua_setfield(L, LUA_REGISTRYINDEX, Name);\
	if (lua_type(L,2) == LUA_TNIL) \
		del_memory_proc(proc##_readlist, addr, proc##_numreadfuncs);\
	else \
		if (add_memory_proc(proc##_readlist, addr, proc##_numreadfuncs)) \
			luaL_error(L, #proc "read hook registry is full.");\
	return 0;\
}\
int memory_register##proc##exec (lua_State* L)\
{\
	unsigned int addr = luaL_checkinteger(L, 1);\
	char Name[16];\
	sprintf(Name,#proc"_E%08X",addr);\
	if (lua_type(L,2) != LUA_TNIL && lua_type(L,2) != LUA_TFUNCTION)\
		luaL_error(L, "function or nil expected in arg 2 to memory.register" #proc "write");\
	lua_setfield(L, LUA_REGISTRYINDEX, Name);\
	if (lua_type(L,2) == LUA_TNIL) \
		del_memory_proc(proc##_execlist, addr, proc##_numexecfuncs);\
	else \
		if (add_memory_proc(proc##_execlist, addr, proc##_numexecfuncs)) \
			luaL_error(L, #proc "exec hook registry is full.");\
	return 0;\
}
memreg(M68K)
memreg(S68K)
//memreg(SH2)
//memreg(Z80)
int gens_registerbefore(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFOREEMULATION]);
	StopScriptIfFinished(luaStateToUIDMap[L]);
	return 0;
}
int gens_registerafter(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_AFTEREMULATION]);
	StopScriptIfFinished(luaStateToUIDMap[L]);
	return 0;
}
int gens_registerexit(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFOREEXIT]);
	//StopScriptIfFinished(luaStateToUIDMap[L]);
	return 0;
}
int gui_register(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_AFTEREMULATIONGUI]);
	StopScriptIfFinished(luaStateToUIDMap[L]);
	return 0;
}
int state_registersave(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFORESAVE]);
	StopScriptIfFinished(luaStateToUIDMap[L]);
	return 0;
}
int state_registerload(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_AFTERLOAD]);
	StopScriptIfFinished(luaStateToUIDMap[L]);
	return 0;
}


static const char* deferredGUIIDString = "lazygui";

// store the most recent C function call from Lua (and all its arguments)
// for later evaluation
void DeferFunctionCall(lua_State* L, const char* idstring)
{
	// there's probably a cleaner way of doing this using lua_pushcclosure or lua_getfenv

	int num = lua_gettop(L);

	// get the C function pointer
	int funcStackIndex = -(num+1);
	lua_CFunction cf = lua_tocfunction(L, funcStackIndex);
	assert(cf); // the above is kind of a hack so it might need to be replaced with the following also-hack if this assert fails:
	//lua_CFunction cf = &(L->ci->func)->value.gc->cl.c.f;
	lua_pushcfunction(L,cf);

	// make a list of the function and its arguments (and also pop those arguments from the stack)
	lua_createtable(L, num+1, 0);
	lua_insert(L, 1);
	for(int n = num+1; n > 0; n--)
		lua_rawseti(L, 1, n);

	// put the list into a global array
	lua_getfield(L, LUA_REGISTRYINDEX, idstring);
	lua_insert(L, 1);
	int curSize = luaL_getn(L, 1);
	lua_rawseti(L, 1, curSize+1);

	// clean the stack
	lua_settop(L, 0);
}
void CallDeferredFunctions(lua_State* L, const char* idstring)
{
	lua_settop(L, 0);
	lua_getfield(L, LUA_REGISTRYINDEX, idstring);
	int numCalls = luaL_getn(L, 1);
	for(int i = 1; i <= numCalls; i++)
	{
        lua_rawgeti(L, 1, i);  // get the function+arguments list
		int listSize = luaL_getn(L, 2);

		// push the arguments and the function
		for(int j = 1; j <= listSize; j++)
			lua_rawgeti(L, 2, j);

		// get and pop the function
		lua_CFunction cf = lua_tocfunction(L, -1);
		lua_pop(L, 1);

		// swap the arguments on the stack with the list we're iterating through
		// before calling the function, because C functions assume argument 1 is at 1 on the stack
		lua_pushvalue(L, 1);
		lua_remove(L, 2);
		lua_remove(L, 1);

		// call the function
		cf(L);
 
		// put the list back where it was
		lua_replace(L, 1);
		lua_settop(L, 1);
	}

	// clear the list of deferred functions
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, idstring);
	LuaContextInfo& info = GetCurrentInfo();
	info.numDeferredGUIFuncs = 0;

	// clean the stack
	lua_settop(L, 0);
}

#define MAX_DEFERRED_COUNT 16384

bool DeferGUIFuncIfNeeded(lua_State* L)
{
	LuaContextInfo& info = GetCurrentInfo();
	if(info.speedMode == SPEEDMODE_MAXIMUM)
	{
		// if the mode is "maximum" then discard all GUI function calls
		// and pretend it was because we deferred them
		return true;
	}
	if(info.guiFuncsNeedDeferring)
	{
		if(info.numDeferredGUIFuncs < MAX_DEFERRED_COUNT)
		{
			// defer whatever function called this one until later
			DeferFunctionCall(L, deferredGUIIDString);
			info.numDeferredGUIFuncs++;
		}
		else
		{
			// too many deferred functions on the same frame
			// silently discard the rest
		}
		return true;
	}

	// ok to run the function right now
	return false;
}

void worry(lua_State* L, int intensity)
{
	LuaContextInfo& info = GetCurrentInfo();
	if(info.worryCount >= 0)
		info.worryCount += intensity;
}

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define APPENDPRINT { int n = snprintf(ptr, remaining,
#define END ); ptr += n; remaining -= n; }
static void toCStringConverter(lua_State* L, int i, char*& ptr, int& remaining)
{
	const char* str = ptr; // for debugging

	switch(lua_type(L, i))
	{
		case LUA_TNONE: APPENDPRINT "no value" END break;
		case LUA_TNIL: APPENDPRINT "nil" END break;
		case LUA_TBOOLEAN: APPENDPRINT lua_toboolean(L,i) ? "true" : "false" END break;
		case LUA_TSTRING: APPENDPRINT "%s",lua_tostring(L,i) END break;
		case LUA_TNUMBER: APPENDPRINT "%Lg",lua_tonumber(L,i) END break;
		default: APPENDPRINT "%s:%p",luaL_typename(L,i),lua_topointer(L,i) END break;
		case LUA_TTABLE:
		{
			APPENDPRINT "{" END
			lua_pushnil(L); // first key
			int keyIndex = lua_gettop(L);
			int valueIndex = keyIndex + 1;
			bool first = true;
			while(lua_next(L, i))
			{
				bool keyIsString = (lua_type(L, keyIndex) == LUA_TSTRING);
				bool invalidLuaIdentifier = (!keyIsString || !isalpha(*lua_tostring(L, keyIndex)));
				if(first)
					first = false;
				else
					APPENDPRINT ", " END
				if(invalidLuaIdentifier)
					if(keyIsString)
						APPENDPRINT "['" END
					else
						APPENDPRINT "[" END

				toCStringConverter(L, keyIndex, ptr, remaining); // key

				if(invalidLuaIdentifier)
					if(keyIsString)
						APPENDPRINT "']=" END
					else
						APPENDPRINT "]=" END
				else
					APPENDPRINT "=" END

				bool valueIsString = (lua_type(L, valueIndex) == LUA_TSTRING);
				if(valueIsString)
					APPENDPRINT "'" END

				toCStringConverter(L, valueIndex, ptr, remaining); // value

				if(valueIsString)
					APPENDPRINT "'" END

				lua_pop(L, 1);
			}
			APPENDPRINT "}" END
		}	break;
	}
}
static const char* toCString(lua_State* L)
{
	static const int maxLen = 4096;
	static char str[maxLen];
	str[0] = 0;
	char* ptr = str;

	int remaining = maxLen;
	int n=lua_gettop(L);
	for(int i = 1; i <= n; i++)
	{
		toCStringConverter(L, i, ptr, remaining);
		if(i != n)
			APPENDPRINT " " END
	}
	APPENDPRINT "\r\n" END

	return str;
}
static const char* toCString(lua_State* L, int i)
{
	luaL_checkany(L,i);
	static const int maxLen = 4096;
	static char str[maxLen];
	str[0] = 0;
	char* ptr = str;
	int remaining = maxLen;
	toCStringConverter(L, i, ptr, remaining);
	return str;
}
#undef APPENDPRINT
#undef END

static int gens_message(lua_State* L)
{
	const char* str = toCString(L);
	Put_Info_NonImmediate((char*)str, 500);
	return 0;
}

static int print(lua_State* L)
{
	int uid = luaStateToUIDMap[L];
	LuaContextInfo& info = *luaContextInfo[uid];

	const char* str = toCString(L);

	if(info.print)
		info.print(uid, str);
	else
		puts(str);

	worry(L, 100);
	return 0;
}

// provides a way to get (from Lua) the string
// that print() or gens.message() would print
static int tostring(lua_State* L)
{
	const char* str = toCString(L);
	lua_pushstring(L, str);
	return 1;
}

static int bitand(lua_State *L)
{
	int rv = ~0;
	int numArgs = lua_gettop(L);
	for(int i = 1; i <= numArgs; i++)
		rv &= luaL_checkinteger(L,i);
	lua_settop(L,0);
	lua_pushinteger(L,rv);
	return 1;
}
static int bitor(lua_State *L)
{
	int rv = 0;
	int numArgs = lua_gettop(L);
	for(int i = 1; i <= numArgs; i++)
		rv |= luaL_checkinteger(L,i);
	lua_settop(L,0);
	lua_pushinteger(L,rv);
	return 1;
}
static int bitxor(lua_State *L)
{
	int rv = 0;
	int numArgs = lua_gettop(L);
	for(int i = 1; i <= numArgs; i++)
		rv ^= luaL_checkinteger(L,i);
	lua_settop(L,0);
	lua_pushinteger(L,rv);
	return 1;
}
static int bitshift(lua_State *L)
{
	int num = luaL_checkinteger(L,1);
	int shift = luaL_checkinteger(L,2);
	if(shift < 0)
		num <<= -shift;
	else
		num >>= shift;
	lua_settop(L,0);
	lua_pushinteger(L,num);
	return 1;
}


#define HOOKCOUNT 4096
#define MAX_WORRY_COUNT 6000
void LuaRescueHook(lua_State* L, lua_Debug *dbg)
{
	LuaContextInfo& info = GetCurrentInfo();

	if(info.worryCount < 0 && !info.panic)
		return;

	info.worryCount++;

	if(info.worryCount > MAX_WORRY_COUNT || info.panic)
	{
		info.worryCount = 0;

		int answer = IDYES;
		if(!info.panic)
		{
#ifdef _WIN32
			DialogsOpen++;
			answer = MessageBox(HWnd, "A Lua script has been running for quite a while. Maybe it is in an infinite loop.\n\nWould you like to stop the script?\n\n(Yes to stop it now,\n No to keep running and not ask again,\n Cancel to keep running but ask again later)", "Lua Alert", MB_YESNOCANCEL | MB_DEFBUTTON3 | MB_ICONASTERISK);
			DialogsOpen--;
#endif
		}

		if(answer == IDNO)
			info.worryCount = -1; // don't remove the hook because we need it still running for RequestAbortLuaScript to work

		if(answer == IDYES)
		{
			//lua_sethook(L, NULL, 0, 0);
			assert(L->errfunc || L->errorJmp);
			luaL_error(L, info.panic ? info.panicMessage : "terminated by user");
		}

		info.panic = false;
	}
}

int gens_emulateframe(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	Update_Emulation_One(HWnd);

	worry(L,300);
	return 0;
}

int gens_emulateframefastnoskipping(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	Update_Emulation_One_Before(HWnd);
	Update_Frame_Hook();
	Update_Emulation_After_Controlled(HWnd, true);

	worry(L,200);
	return 0;
}

int gens_emulateframefast(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	disableVideoLatencyCompensationCount = VideoLatencyCompensation + 1;

	Update_Emulation_One_Before(HWnd);

	if(FrameCount%16 == 0) // skip rendering 15 out of 16 frames
	{
		// update once and render
		Update_Frame_Hook();
		Update_Emulation_After_Controlled(HWnd, true);
	}
	else
	{
		// update once but skip rendering
		Update_Frame_Fast_Hook();
		Update_Emulation_After_Controlled(HWnd, false);
	}

	worry(L,150);
	return 0;
}

int gens_emulateframeinvisible(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	int oldDisableSound2 = disableSound2;
	int oldDisableRamSearchUpdate = disableRamSearchUpdate;
	disableSound2 = true;
	disableRamSearchUpdate = true;

	Update_Emulation_One_Before_Minimal();
	Update_Frame_Fast();
	UpdateLagCount();

	disableSound2 = oldDisableSound2;
	disableRamSearchUpdate = oldDisableRamSearchUpdate;

	// disable video latency compensation for a few frames
	// because it can get pretty slow if that's doing prediction updates every frame
	// when the lua script is also doing prediction updates
	disableVideoLatencyCompensationCount = VideoLatencyCompensation + 1;

	worry(L,100);
	return 0;
}

#ifndef _WIN32
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif

int gens_speedmode(lua_State* L)
{
	SpeedMode newSpeedMode = SPEEDMODE_NORMAL;
	if(lua_isnumber(L,1))
		newSpeedMode = (SpeedMode)luaL_checkinteger(L,1);
	else
	{
		const char* str = luaL_checkstring(L,1);
		if(!stricmp(str, "normal"))
			newSpeedMode = SPEEDMODE_NORMAL;
		else if(!stricmp(str, "nothrottle"))
			newSpeedMode = SPEEDMODE_NOTHROTTLE;
		else if(!stricmp(str, "turbo"))
			newSpeedMode = SPEEDMODE_TURBO;
		else if(!stricmp(str, "maximum"))
			newSpeedMode = SPEEDMODE_MAXIMUM;
	}

	LuaContextInfo& info = GetCurrentInfo();
	info.speedMode = newSpeedMode;
	return 0;
}

int gens_wait(lua_State* L)
{
	LuaContextInfo& info = GetCurrentInfo();

	switch(info.speedMode)
	{
		default:
		case SPEEDMODE_NORMAL:
			while(!Step_Gens_MainLoop(true, false) && !info.panic);
			break;
		case SPEEDMODE_NOTHROTTLE:
		case SPEEDMODE_TURBO:
		case SPEEDMODE_MAXIMUM:
			while(!Step_Gens_MainLoop(Paused!=0, false) && !info.panic);
			break;
	}
	return 0;
}

int gens_frameadvance(lua_State* L)
{
	int uid = luaStateToUIDMap[L];
	LuaContextInfo& info = GetCurrentInfo();

	if(!info.ranFrameAdvance)
	{
		// otherwise we'll never see the first frame of GUI drawing
		if(info.speedMode != SPEEDMODE_MAXIMUM)
			Show_Genesis_Screen();
		info.ranFrameAdvance = true;
	}

	switch(info.speedMode)
	{
		default:
		case SPEEDMODE_NORMAL:
			while(!Step_Gens_MainLoop(true, true) && !info.panic);
			break;
		case SPEEDMODE_NOTHROTTLE:
			while(!Step_Gens_MainLoop(Paused!=0, false) && !info.panic);
			if(!(FastForwardKeyDown && (GetActiveWindow()==HWnd || BackgroundInput)))
				gens_emulateframefastnoskipping(L);
			else
				gens_emulateframefast(L);
			break;
		case SPEEDMODE_TURBO:
			while(!Step_Gens_MainLoop(Paused!=0, false) && !info.panic);
			gens_emulateframefast(L);
			break;
		case SPEEDMODE_MAXIMUM:
			while(!Step_Gens_MainLoop(Paused!=0, false) && !info.panic);
			gens_emulateframeinvisible(L);
			break;
	}
	return 0;
}

int gens_pause(lua_State* L)
{
	Paused = 1;
	gens_frameadvance(L);

	// allow the user to not have to manually unpause
	// after restarting a script that used gens.pause()
	LuaContextInfo& info = GetCurrentInfo();
	if(info.panic)
		Paused = 0;

	return 0;
}

int gens_redraw(lua_State* L)
{
	Show_Genesis_Screen();
	worry(L,250);
	return 0;
}




int memory_readbyte(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned char value = (unsigned char)(ReadValueAtHardwareAddress(address, 1) & 0xFF);
	lua_settop(L,0);
	lua_pushinteger(L, value);
	return 1; // we return the number of return values
}
int memory_readbytesigned(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	signed char value = (signed char)(ReadValueAtHardwareAddress(address, 1) & 0xFF);
	lua_settop(L,0);
	lua_pushinteger(L, value);
	return 1;
}
int memory_readword(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned short value = (unsigned short)(ReadValueAtHardwareAddress(address, 2) & 0xFFFF);
	lua_settop(L,0);
	lua_pushinteger(L, value);
	return 1;
}
int memory_readwordsigned(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	signed short value = (signed short)(ReadValueAtHardwareAddress(address, 2) & 0xFFFF);
	lua_settop(L,0);
	lua_pushinteger(L, value);
	return 1;
}
int memory_readdword(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned long value = (unsigned long)(ReadValueAtHardwareAddress(address, 4));
	lua_settop(L,0);
	lua_pushinteger(L, value);
	return 1;
}
int memory_readdwordsigned(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	signed long value = (signed long)(ReadValueAtHardwareAddress(address, 4));
	lua_settop(L,0);
	lua_pushinteger(L, value);
	return 1;
}

int memory_writebyte(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned char value = (unsigned char)(luaL_checkinteger(L,2) & 0xFF);
	WriteValueAtHardwareAdress(address, value, 1);
	return 0;
}
int memory_writeword(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned short value = (unsigned short)(luaL_checkinteger(L,2) & 0xFFFF);
	WriteValueAtHardwareAdress(address, value, 2);
	return 0;
}
int memory_writedword(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned long value = (unsigned long)(luaL_checkinteger(L,2));
	WriteValueAtHardwareAdress(address, value, 4);
	return 0;
}

int state_create(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
	{
		lua_pushnil(L);
		return 1;
	}

	int len = GENESIS_STATE_LENGTH;
	if (SegaCD_Started) len += SEGACD_LENGTH_EX;
	if (_32X_Started) len += G32X_LENGTH_EX;

	void* stateBuffer = lua_newuserdata(L, len);
	memset(stateBuffer, 0, len);

	return 1;
}
int state_get(lua_State* L)
{
	void* stateBuffer = lua_touserdata(L,1);

	if(stateBuffer)
		Save_State_To_Buffer((unsigned char*)stateBuffer);

	return 1;
}
int state_set(lua_State* L)
{
	void* stateBuffer = lua_touserdata(L,1);

	if(stateBuffer)
		Load_State_From_Buffer((unsigned char*)stateBuffer);

	return 0;
}
int state_save(lua_State* L)
{
	int stateNumber = luaL_checkinteger(L,1);
	Set_Current_State(stateNumber);
	Str_Tmp[0] = 0;
	Get_State_File_Name(Str_Tmp);
	Save_State(Str_Tmp);
	return 0;
}
int state_load(lua_State* L)
{
	int stateNumber = luaL_checkinteger(L,1);
	Set_Current_State(stateNumber);
	Str_Tmp[0] = 0;
	Get_State_File_Name(Str_Tmp);
	Load_State(Str_Tmp);
	return 0;
}

static const struct ButtonDesc
{
	unsigned short controllerNum;
	unsigned short bit;
	const char* name;
}
s_buttonDescs [] =
{
	{1, 0, "up"},
	{1, 1, "down"},
	{1, 2, "left"},
	{1, 3, "right"},
	{1, 4, "A"},
	{1, 5, "B"},
	{1, 6, "C"},
	{1, 7, "start"},
	{1, 32, "X"},
	{1, 33, "Y"},
	{1, 34, "Z"},
	{1, 35, "mode"},
	{2, 24, "up"},
	{2, 25, "down"},
	{2, 26, "left"},
	{2, 27, "right"},
	{2, 28, "A"},
	{2, 29, "B"},
	{2, 30, "C"},
	{2, 31, "start"},
	{2, 36, "X"},
	{2, 37, "Y"},
	{2, 38, "Z"},
	{2, 39, "mode"},
	{0x1B, 8, "up"},
	{0x1B, 9, "down"},
	{0x1B, 10, "left"},
	{0x1B, 11, "right"},
	{0x1B, 12, "A"},
	{0x1B, 13, "B"},
	{0x1B, 14, "C"},
	{0x1B, 15, "start"},
	{0x1C, 16, "up"},
	{0x1C, 17, "down"},
	{0x1C, 18, "left"},
	{0x1C, 19, "right"},
	{0x1C, 20, "A"},
	{0x1C, 21, "B"},
	{0x1C, 22, "C"},
	{0x1C, 23, "start"},
};

int joy_getArgControllerNum(lua_State* L, int& index)
{
	int controllerNumber;
	int type = lua_type(L,index);
	if(type == LUA_TSTRING || type == LUA_TNUMBER)
	{
		controllerNumber = 0;
		if(type == LUA_TSTRING)
		{
			const char* str = lua_tostring(L,index);
			if(!stricmp(str, "1C"))
				controllerNumber = 0x1C;
			else if(!stricmp(str, "1B"))
				controllerNumber = 0x1B;
			else if(!stricmp(str, "1A"))
				controllerNumber = 0x1A;
		}
		if(!controllerNumber)
			controllerNumber = luaL_checkinteger(L,index);
		index++;
	}
	else
	{
		// argument omitted; default to controller 1
		controllerNumber = 1;
	}

	if(controllerNumber == 0x1A)
		controllerNumber = 1;
	if(controllerNumber != 1 && controllerNumber != 2 && controllerNumber != 0x1B && controllerNumber != 0x1C)
		luaL_error(L, "controller number must be 1, 2, '1B', or '1C'");

	return controllerNumber;
}


// joypad.set(controllerNum = 1, inputTable)
// controllerNum can be 1, 2, '1B', or '1C'
int joy_set(lua_State* L)
{
	int index = 1;
	int controllerNumber = joy_getArgControllerNum(L, index);

	luaL_checktype(L, index, LUA_TTABLE);

	int input = ~0;
	int mask = 0;

	for(int i = 0; i < sizeof(s_buttonDescs)/sizeof(*s_buttonDescs); i++)
	{
		const ButtonDesc& bd = s_buttonDescs[i];
		if(bd.controllerNum == controllerNumber)
		{
			lua_getfield(L, index, bd.name);
			if (!lua_isnil(L,-1))
			{
				bool pressed = lua_toboolean(L,-1) != 0;
				int bitmask = ((long long)1 << bd.bit);
				if(pressed)
					input &= ~bitmask;
				else
					input |= bitmask;
				mask |= bitmask;
			}
			lua_pop(L,1);
		}
	}

	SetNextInputCondensed(input, mask);

	return 0;
}

// joypad.get(controllerNum = 1)
// controllerNum can be 1, 2, '1B', or '1C'
int joy_get_internal(lua_State* L, bool reportUp, bool reportDown)
{
	int index = 1;
	int controllerNumber = joy_getArgControllerNum(L, index);

	lua_newtable(L);

	long long input = GetCurrentInputCondensed();

	for(int i = 0; i < sizeof(s_buttonDescs)/sizeof(*s_buttonDescs); i++)
	{
		const ButtonDesc& bd = s_buttonDescs[i];
		if(bd.controllerNum == controllerNumber)
		{
			bool pressed = (input & ((long long)1<<bd.bit)) == 0;
			if((pressed && reportDown) || (!pressed && reportUp))
			{
				lua_pushboolean(L, pressed);
				lua_setfield(L, -2, bd.name);
			}
		}
	}

	return 1;
}
// joypad.get(int controllerNumber = 1)
// returns a table of every game button,
// true meaning currently-held and false meaning not-currently-held
// this WILL read input from a currently-playing movie
int joy_get(lua_State* L)
{
	return joy_get_internal(L, true, true);
}
// joypad.getdown(int controllerNumber = 1)
// returns a table of every game button that is currently held
int joy_getdown(lua_State* L)
{
	return joy_get_internal(L, false, true);
}
// joypad.getup(int controllerNumber = 1)
// returns a table of every game button that is not currently held
int joy_getup(lua_State* L)
{
	return joy_get_internal(L, true, false);
}

static const struct ColorMapping
{
	const char* name;
	int value;
}
s_colorMapping [] =
{
	{"white",     0xFFFFFFFF},
	{"black",     0x000000FF},
	{"clear",     0x00000000},
	{"gray",      0x7F7F7FFF},
	{"grey",      0x7F7F7FFF},
	{"red",       0xFF0000FF},
	{"orange",    0xFF7F00FF},
	{"yellow",    0xFFFF00FF},
	{"chartreuse",0x7FFF00FF},
	{"green",     0x00FF00FF},
	{"teal",      0x00FF7FFF},
	{"cyan" ,     0x00FFFFFF},
	{"blue",      0x0000FFFF},
	{"purple",    0x7F00FFFF},
	{"magenta",   0xFF00FFFF},
};

inline int getcolor_unmodified(lua_State *L, int idx, int defaultColor)
{
	int type = lua_type(L,idx);
	switch(type)
	{
		case LUA_TSTRING:
		{
			const char* str = lua_tostring(L,idx);
			if(*str == '#')
			{
				int color;
				sscanf(str+1, "%X", &color);
				int len = strlen(str+1);
				int missing = max(0, 8-len);
				color <<= missing << 2;
				if(missing >= 2) color |= 0xFF;
				return color;
			}
			else for(int i = 0; i<sizeof(s_colorMapping)/sizeof(*s_colorMapping); i++)
			{
				if(!stricmp(str,s_colorMapping[i].name))
					return s_colorMapping[i].value;
			}
			if(!strnicmp(str, "rand", 4))
				return ((rand()*255/RAND_MAX) << 8) | ((rand()*255/RAND_MAX) << 16) | ((rand()*255/RAND_MAX) << 24) | 0xFF;
		}	break;
		case LUA_TNUMBER:
		{
			return lua_tointeger(L,idx);
		}	break;
		case LUA_TTABLE:
		{
			int color = 0xFF;
			lua_pushnil(L); // first key
			int keyIndex = lua_gettop(L);
			int valueIndex = keyIndex + 1;
			bool first = true;
			while(lua_next(L, idx))
			{
				bool keyIsString = (lua_type(L, keyIndex) == LUA_TSTRING);
				bool valIsNumber = (lua_type(L, valueIndex) == LUA_TNUMBER);
				if(keyIsString)
				{
					const char* key = lua_tostring(L, keyIndex);
					int value = lua_tointeger(L, valueIndex);
					if(value < 0) value = 0;
					if(value > 255) value = 255;
					switch(tolower(*key))
					{
					case 'r':
						color |= value << 24;
						break;
					case 'g':
						color |= value << 16;
						break;
					case 'b':
						color |= value << 8;
						break;
					case 'a':
						color = (color & ~0xFF) | value;
						break;
					}
				}
				lua_pop(L, 1);
			}
			return color;
		}	break;
	}
	return defaultColor;
}
int getcolor(lua_State *L, int idx, int defaultColor)
{
	int color = getcolor_unmodified(L, idx, defaultColor);
	LuaContextInfo& info = GetCurrentInfo();
	if(info.transparencyModifier != 255)
	{
		int alpha = (((color & 0xFF) * info.transparencyModifier) / 255);
		if(alpha > 255) alpha = 255;
		color = (color & ~0xFF) | alpha;
	}
	return color;
}

int gui_text(lua_State* L)
{
	if(DeferGUIFuncIfNeeded(L))
		return 0; // we have to wait until later to call this function because gens hasn't emulated the next frame yet
		          // (the only way to avoid this deferring is to be in a gui.register or gens.registerafter callback)

	int x = luaL_checkinteger(L,1) & 0xFFFF;
	int y = luaL_checkinteger(L,2) & 0xFFFF;
	const char* str = toCString(L,3); // better than using luaL_checkstring here (more permissive)
	
	if(str && *str)
	{
		int foreColor = getcolor(L,4,0xFFFFFFFF);
		int backColor = getcolor(L,5,0x000000FF);
		PutText2(str, x, y, foreColor, backColor);
	}

	return 0;
}
int gui_box(lua_State* L)
{
	if(DeferGUIFuncIfNeeded(L))
		return 0;

	int x1 = luaL_checkinteger(L,1) & 0xFFFF;
	int y1 = luaL_checkinteger(L,2) & 0xFFFF;
	int x2 = luaL_checkinteger(L,3) & 0xFFFF;
	int y2 = luaL_checkinteger(L,4) & 0xFFFF;
	int fillcolor = getcolor(L,5,0xFFFFFF3F);
	int outlinecolor = getcolor(L,6,0xFFFFFFFF);

	DrawBoxPP2 (x1, y1, x2, y2, fillcolor, outlinecolor);

	return 0;
}
// gui.setpixel(x,y,color)
// color can be a RGB web color like '#ff7030', or with alpha RGBA like '#ff703060'
//   or it can be an RGBA hex number like 0xFF703060
//   or it can be a preset color like 'red', 'orange', 'blue', 'white', etc.
int gui_pixel(lua_State* L)
{
	if(DeferGUIFuncIfNeeded(L))
		return 0;

	int x = luaL_checkinteger(L,1) & 0xFFFF;
	int y = luaL_checkinteger(L,2) & 0xFFFF;
	int color = getcolor(L,3,0xFFFFFFFF);
	int color32 = color>>8;
	int color16 = DrawUtil::Pix32To16(color32);
	int Opac = color & 0xFF;

	if(Opac)
		Pixel(x, y, color32, color16, 0, Opac);

	return 0;
}
// r,g,b = gui.getpixel(x,y)
int gui_getpixel(lua_State* L)
{
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);

	int xres = ((VDP_Reg.Set4 & 0x1) || Debug || !Game || !FrameCount) ? 320 : 256;
	int yres = ((VDP_Reg.Set2 & 0x8) || Debug || !Game || !FrameCount) ? 240 : 224;

	x = max(0,min(xres,x));
	y = max(0,min(yres,y));

	int off = (y * 336) + x + 8;

	int color;
	if (Bits32)
		color = MD_Screen32[off];
	else
		color = DrawUtil::Pix16To32(MD_Screen[off]);

	int b = (color & 0x000000FF);
	int g = (color & 0x0000FF00) >> 8;
	int r = (color & 0x00FF0000) >> 16;

	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);

	return 3;
}
int gui_line(lua_State* L)
{
	if(DeferGUIFuncIfNeeded(L))
		return 0;

	int x1 = luaL_checkinteger(L,1) & 0xFFFF;
	int y1 = luaL_checkinteger(L,2) & 0xFFFF;
	int x2 = luaL_checkinteger(L,3) & 0xFFFF;
	int y2 = luaL_checkinteger(L,4) & 0xFFFF;
	int color = getcolor(L,5,0xFFFFFFFF);
	int color32 = color>>8;
	int color16 = DrawUtil::Pix32To16(color32);
	int Opac = color & 0xFF;

	if(Opac)
		DrawLine(x1, y1, x2, y2, color32, color16, 0, Opac);

	return 0;
}

// gui.opacity(number alphaValue)
// sets the transparency of subsequent draw calls
// 0.0 is completely transparent, 1.0 is completely opaque
// non-integer values are supported and meaningful, as are values greater than 1.0
// it is not necessary to use this function to get transparency (or the less-recommended gui.transparency() either),
// because you can provide an alpha value in the color argument of each draw call.
// however, it can be convenient to be able to globally modify the drawing transparency
int gui_setopacity(lua_State* L)
{
	lua_Number opacF = luaL_checknumber(L,1);
	opacF *= 255.0;
	if(opacF < 0) opacF = 0;
	int opac;
	lua_number2int(opac, opacF);
	LuaContextInfo& info = GetCurrentInfo();
	info.transparencyModifier = opac;
	return 0;
}

// gui.transparency(number transparencyValue)
// sets the transparency of subsequent draw calls
// 0.0 is completely opaque, 4.0 is completely transparent
// non-integer values are supported and meaningful, as are values less than 0.0
// this is a legacy funciton, and the range is from 0 to 4 solely for this reason
// it does the exact same thing as gui.opacity() but with a different argument range
int gui_settransparency(lua_State* L)
{
	lua_Number transp = luaL_checknumber(L,1);
	lua_Number opacF = 4 - transp;
	opacF *= 255.0 / 4.0;
	if(opacF < 0) opacF = 0;
	int opac;
	lua_number2int(opac, opacF);
	LuaContextInfo& info = GetCurrentInfo();
	info.transparencyModifier = opac;
	return 0;
}

int gens_getframecount(lua_State* L)
{
	lua_pushinteger(L, FrameCount);
	return 1;
}
int gens_getlagcount(lua_State* L)
{
	lua_pushinteger(L, LagCountPersistent);
	return 1;
}
int movie_getlength(lua_State* L)
{
	lua_pushinteger(L, MainMovie.LastFrame);
	return 1;
}
int movie_isactive(lua_State* L)
{
	lua_pushboolean(L, MainMovie.File != NULL);
	return 1;
}
int movie_rerecordcount(lua_State* L)
{
	lua_pushinteger(L, MainMovie.NbRerecords);
	return 1;
}
int movie_getreadonly(lua_State* L)
{
	lua_pushboolean(L, MainMovie.ReadOnly);
	return 1;
}
int movie_setreadonly(lua_State* L)
{
	bool readonly = lua_toboolean(L,1) != 0;
	MainMovie.ReadOnly = readonly;
	return 0;
}
int movie_isrecording(lua_State* L)
{
	lua_pushboolean(L, MainMovie.Status == MOVIE_RECORDING);
	return 1;
}
int movie_isplaying(lua_State* L)
{
	lua_pushboolean(L, MainMovie.Status == MOVIE_PLAYING);
	return 1;
}
int movie_getmode(lua_State* L)
{
	switch(MainMovie.Status)
	{
	case MOVIE_PLAYING:
		lua_pushstring(L, "playback");
		break;
	case MOVIE_RECORDING:
		lua_pushstring(L, "record");
		break;
	case MOVIE_FINISHED:
		lua_pushstring(L, "finished");
		break;
	default:
		lua_pushnil(L);
		break;
	}
	return 1;
}
int movie_getname(lua_State* L)
{
	lua_pushstring(L, MainMovie.FileName);
	return 1;
}
// movie.play() -- plays a movie of the user's choice
// movie.play(filename) -- starts playing a particular movie
// throws an error (with a description) if for whatever reason the movie couldn't be played
int movie_play(lua_State *L)
{
	const char* filename = lua_isstring(L,1) ? lua_tostring(L,1) : NULL;
	const char* errorMsg = GensPlayMovie(filename, true);
	if(errorMsg)
		luaL_error(L, errorMsg);
    return 0;
} 
int movie_replay(lua_State *L)
{
	GensReplayMovie();
    return 0;
} 
int movie_close(lua_State* L)
{
	CloseMovieFile(&MainMovie);
	return 0;
}

int sound_clear(lua_State* L)
{
	Clear_Sound_Buffer();
	return 0;
}

#ifdef _WIN32
const char* s_keyToName[256] =
{
	NULL,
	"leftclick",
	"rightclick",
	NULL,
	"middleclick",
	NULL,
	NULL,
	NULL,
	"backspace",
	"tab",
	NULL,
	NULL,
	NULL,
	"enter",
	NULL,
	NULL,
	"shift", // 0x10
	"control",
	"alt",
	"pause",
	"capslock",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"escape",
	NULL,
	NULL,
	NULL,
	NULL,
	"space", // 0x20
	"pageup",
	"pagedown",
	"end",
	"home",
	"left",
	"up",
	"right",
	"down",
	NULL,
	NULL,
	NULL,
	NULL,
	"insert",
	"delete",
	NULL,
	"0","1","2","3","4","5","6","7","8","9",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"A","B","C","D","E","F","G","H","I","J",
	"K","L","M","N","O","P","Q","R","S","T",
	"U","V","W","X","Y","Z",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"numpad0","numpad1","numpad2","numpad3","numpad4","numpad5","numpad6","numpad7","numpad8","numpad9",
	"numpad*","numpad+",
	NULL,
	"numpad-","numpad.","numpad/",
	"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
	"F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23","F24",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"numlock",
	"scrolllock",
	NULL, // 0x92
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, // 0xB9
	"semicolon",
	"plus",
	"comma",
	"minus",
	"period",
	"slash",
	"tilde",
	NULL, // 0xC1
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, // 0xDA
	"leftbracket",
	"backslash",
	"rightbracket",
	"quote",
};
#endif


// input.get()
// takes no input, returns a lua table of entries representing the current input state,
// independent of the joypad buttons the emulated game thinks are pressed
// for example:
//   if the user is holding the W key and the left mouse button
//   and has the mouse at the bottom-right corner of the game screen,
//   then this would return {W=true, LeftClick=true, MouseX=319, MouseY=223}
int input_getcurrentinputstatus(lua_State* L)
{
	lua_newtable(L);

#ifdef _WIN32
	// keyboard and mouse button status
	{
		unsigned char keys [256];
		if(!BackgroundInput)
		{
			if(GetKeyboardState(keys))
			{
				for(int i = 1; i < 255; i++)
				{
					int mask = (i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL) ? 0x01 : 0x80;
					if(keys[i] & mask)
					{
						const char* name = s_keyToName[i];
						if(name)
						{
							lua_pushboolean(L, true);
							lua_setfield(L, -2, name);
						}
					}
				}
			}
		}
		else // use a slightly different method that will detect background input:
		{
			for(int i = 1; i < 255; i++)
			{
				const char* name = s_keyToName[i];
				if(name)
				{
					int active;
					if(i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL)
						active = GetKeyState(i) & 0x01;
					else
						active = GetAsyncKeyState(i) & 0x8000;
					if(active)
					{
						lua_pushboolean(L, true);
						lua_setfield(L, -2, name);
					}
				}
			}
		}
	}
	// mouse position in game screen pixel coordinates
	{
		POINT point;
		RECT rect, srcRectUnused;
		float xRatioUnused, yRatioUnused;
		int depUnused;
		GetCursorPos(&point);
		ScreenToClient(HWnd, &point);
		GetClientRect(HWnd, &rect);
		void CalculateDrawArea(int Render_Mode, RECT& RectDest, RECT& RectSrc, float& Ratio_X, float& Ratio_Y, int& Dep);
		CalculateDrawArea(Full_Screen ? Render_FS : Render_W, rect, srcRectUnused, xRatioUnused, yRatioUnused, depUnused);
		int xres = ((VDP_Reg.Set4 & 0x1) || Debug || !Game || !FrameCount) ? 320 : 256;
		int yres = ((VDP_Reg.Set2 & 0x8) || Debug || !Game || !FrameCount) ? 240 : 224;
		int x = ((point.x - rect.left) * xres) / (rect.right - rect.left);
		int y = ((point.y - rect.top) * yres) / (rect.bottom - rect.top);
		lua_pushinteger(L, x);
		lua_setfield(L, -2, "xmouse");
		lua_pushinteger(L, y);
		lua_setfield(L, -2, "ymouse");
	}
#else
	// NYI (well, return an empty table)
#endif

	return 1;
}


// resets our "worry" counter of the Lua state
int dontworry(LuaContextInfo& info)
{
	info.worryCount = min(info.worryCount, 0);
	return 0;
}

static const struct luaL_reg genslib [] =
{
	{"frameadvance", gens_frameadvance},
	{"speedmode", gens_speedmode},
	{"wait", gens_wait},
	{"pause", gens_pause},
	{"emulateframe", gens_emulateframe},
	{"emulateframefastnoskipping", gens_emulateframefastnoskipping},
	{"emulateframefast", gens_emulateframefast},
	{"emulateframeinvisible", gens_emulateframeinvisible},
	{"redraw", gens_redraw},
	{"framecount", gens_getframecount},
	{"lagcount", gens_getlagcount},
	{"registerbefore", gens_registerbefore},
	{"registerafter", gens_registerafter},
	{"registerexit", gens_registerexit},
	{"message", gens_message},
	{NULL, NULL}
};
static const struct luaL_reg guilib [] =
{
	{"register", gui_register},
	{"text", gui_text},
	{"box", gui_box},
	{"line", gui_line},
	{"pixel", gui_pixel},
	{"getpixel", gui_getpixel},
	{"opacity", gui_setopacity},
	{"transparency", gui_settransparency},
	// alternative names
	{"drawtext", gui_text},
	{"drawbox", gui_box},
	{"drawline", gui_line},
	{"drawpixel", gui_pixel},
	{"rect", gui_box},
	{"drawrect", gui_box},
	{NULL, NULL}
};
static const struct luaL_reg statelib [] =
{
	{"create", state_create},
	{"set", state_set},
	{"get", state_get},
	{"save", state_save},
	{"load", state_load},
	{"registersave", state_registersave},
	{"registerload", state_registerload},
	{NULL, NULL}
};
static const struct luaL_reg memorylib [] =
{
	{"readbyte", memory_readbyte},
	{"readbyteunsigned", memory_readbyte},
	{"readbytesigned", memory_readbytesigned},
	{"readword", memory_readword},
	{"readwordunsigned", memory_readword},
	{"readwordsigned", memory_readwordsigned},
	{"readdword", memory_readdword},
	{"readdwordunsigned", memory_readdword},
	{"readdwordsigned", memory_readdwordsigned},
	{"writebyte", memory_writebyte},
	{"writeword", memory_writeword},
	{"writedword", memory_writedword},
	// alternate naming scheme for word and double-word
	{"readshort", memory_readword},
	{"readshortunsigned", memory_readword},
	{"readshortsigned", memory_readwordsigned},
	{"readlong", memory_readdword},
	{"readlongunsigned", memory_readdword},
	{"readlongsigned", memory_readdwordsigned},
	{"writeshort", memory_writeword},
	{"writelong", memory_writedword},

	//main 68000 memory hooks
	{"register", memory_registerM68Kwrite},
	{"registerwrite", memory_registerM68Kwrite},
	{"registerread", memory_registerM68Kread},
	{"registerexec", memory_registerM68Kexec},

	//full names for main 68000 memory hooks
	{"registerM68K", memory_registerM68Kwrite},
	{"registerM68Kwrite", memory_registerM68Kwrite},
	{"registerM68Kread", memory_registerM68Kread},
	{"registerM68Kexec", memory_registerM68Kexec},

	//alternate names for main 68000 memory hooks
	{"registergen", memory_registerM68Kwrite},
	{"registergenwrite", memory_registerM68Kwrite},
	{"registergenread", memory_registerM68Kread},
	{"registergenexec", memory_registerM68Kexec},

	//sub 68000 (segaCD) memory hooks
	{"registerS68K", memory_registerS68Kwrite},
	{"registerS68Kwrite", memory_registerS68Kwrite},
	{"registerS68Kread", memory_registerS68Kread},
	{"registerS68Kexec", memory_registerS68Kexec},

	//alternate names for sub 68000 (segaCD) memory hooks
	{"registerCD", memory_registerS68Kwrite},
	{"registerCDwrite", memory_registerS68Kwrite},
	{"registerCDread", memory_registerS68Kread},
	{"registerCDexec", memory_registerS68Kexec},

//	//Super-H 2 (32X) memory hooks
//	{"registerSH2", memory_registerSH2write},
//	{"registerSH2write", memory_registerSH2write},
//	{"registerSH2read", memory_registerSH2read},
//	{"registerSH2exec", memory_registerSH2PC},
//	{"registerSH2PC", memory_registerSH2PC},

//	//alternate names for Super-H 2 (32X) memory hooks
//	{"register32X", memory_registerSH2write},
//	{"register32Xwrite", memory_registerSH2write},
//	{"register32Xread", memory_registerSH2read},
//	{"register32Xexec", memory_registerSH2PC},
//	{"register32XPC", memory_registerSH2PC},

//	//Z80 (sound controller) memory hooks
//	{"registerZ80", memory_registerZ80write},
//	{"registerZ80write", memory_registerZ80write},
//	{"registerZ80read", memory_registerZ80read},
//	{"registerZ80PC", memory_registerZ80PC},

	{NULL, NULL}
};
static const struct luaL_reg joylib [] =
{
	{"get", joy_get},
	{"set", joy_set},
	{"getdown", joy_getdown},
	{"getup", joy_getup},
	// alternative names
	{"read", joy_get},
	{"write", joy_set},
	{"readdown", joy_getdown},
	{"readup", joy_getup},
	{NULL, NULL}
};
static const struct luaL_reg inputlib [] =
{
	{"get", input_getcurrentinputstatus},
	// alternative names
	{"read", input_getcurrentinputstatus},
	{NULL, NULL}
};
static const struct luaL_reg movielib [] =
{
	{"length", movie_getlength},
	{"active", movie_isactive},
	{"recording", movie_isrecording},
	{"playing", movie_isplaying},
	{"mode", movie_getmode},
	{"name", movie_getname},
	{"getname", movie_getname},
	{"rerecordcount", movie_rerecordcount},
	{"readonly", movie_getreadonly},
	{"getreadonly", movie_getreadonly},
	{"setreadonly", movie_setreadonly},
	{"playback", movie_play},
	{"play", movie_play},
	{"replay", movie_replay},
	{"stop", movie_close},
	// alternative names
	{"open", movie_play},
	{"close", movie_close},
	{NULL, NULL}
};
static const struct luaL_reg soundlib [] =
{
	{"clear", sound_clear},
	{NULL, NULL}
};

void ResetInfo(LuaContextInfo& info)
{
	info.L = NULL;
	info.started = false;
	info.running = false;
	info.returned = false;
	info.crashed = false;
	info.restart = false;
	info.restartLater = false;
	info.worryCount = 0;
	info.panic = false;
	info.ranExit = false;
	info.numDeferredGUIFuncs = 0;
	info.ranFrameAdvance = false;
	info.transparencyModifier = 255;
	info.speedMode = SPEEDMODE_NORMAL;
	info.guiFuncsNeedDeferring = false;
}

void OpenLuaContext(int uid, void(*print)(int uid, const char* str), void(*onstart)(int uid), void(*onstop)(int uid, bool statusOK))
{
	LuaContextInfo* newInfo = new LuaContextInfo();
	ResetInfo(*newInfo);
	newInfo->print = print;
	newInfo->onstart = onstart;
	newInfo->onstop = onstop;
	luaContextInfo[uid] = newInfo;
}

void RefreshScriptStartedStatus();

void RunLuaScriptFile(int uid, const char* filenameCStr)
{
	StopLuaScript(uid);

	LuaContextInfo& info = *luaContextInfo[uid];

#ifdef USE_INFO_STACK
	infoStack.insert(infoStack.begin(), &info);
	struct Scope { ~Scope(){ infoStack.erase(infoStack.begin()); } } scope;
#endif

	if(info.running)
	{
		// it's a little complicated, but... the call to luaL_dofile below
		// could call a C function that calls this very function again
		// additionally, if that happened then the above call to StopLuaScript
		// probably couldn't stop the script yet, so instead of continuing,
		// we'll set a flag that tells the first call of this function to loop again
		// when the script is able to stop safely
		info.restart = true;
		return;
	}

	std::string filename = filenameCStr;

	do
	{
		lua_State* L = lua_open();
#ifndef USE_INFO_STACK
		luaStateToContextMap[L] = &info;
#endif
		luaStateToUIDMap[L] = uid;
		ResetInfo(info);
		info.L = L;
		info.guiFuncsNeedDeferring = true;
		info.lastFilename = filename;

		luaL_openlibs(L);
		luaL_register(L, "gens", genslib);
		luaL_register(L, "gui", guilib);
		luaL_register(L, "savestate", statelib);
		luaL_register(L, "memory", memorylib);
		luaL_register(L, "joypad", joylib); // for game input
		luaL_register(L, "input", inputlib); // for user input
		luaL_register(L, "movie", movielib);
		luaL_register(L, "sound", soundlib);
		lua_register(L, "print", print);
		lua_register(L, "tostring", tostring);
		lua_register(L, "AND", bitand);
		lua_register(L, "OR", bitor);
		lua_register(L, "XOR", bitxor);
		lua_register(L, "SHIFT", bitshift);

		// register a function to periodically check for inactivity
		lua_sethook(L, LuaRescueHook, LUA_MASKCOUNT, HOOKCOUNT);

		// deferred evaluation table
		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, deferredGUIIDString);

		info.started = true;
		RefreshScriptStartedStatus();
		if(info.onstart)
			info.onstart(uid);
		info.running = true;
		info.returned = false;
		int errorcode = luaL_dofile(L,filename.c_str());
		info.running = false;
		info.returned = true;

		if (errorcode)
		{
			info.crashed = true;
			if(info.print)
			{
				info.print(uid, lua_tostring(L,-1));
				info.print(uid, "\r\n");
			}
			else
			{
				fprintf(stderr, "%s\n", lua_tostring(L,-1));
			}
			StopLuaScript(uid);
		}
		else
		{
			Show_Genesis_Screen();
			StopScriptIfFinished(uid, true);
		}
	} while(info.restart);
}

void StopScriptIfFinished(int uid, bool justReturned)
{
	LuaContextInfo& info = *luaContextInfo[uid];
	if(!info.returned)
		return;

	// the script has returned, but it is not necessarily done running
	// because it may have registered a function that it expects to keep getting called
	// so check if it has any registered functions and stop the script only if it doesn't

	bool keepAlive = false;
	for(int calltype = 0; calltype < LUACALL_COUNT; calltype++)
	{
		if(calltype == LUACALL_BEFOREEXIT) // this is probably the only one that shouldn't keep the script alive
			continue;

		lua_State* L = info.L;
		if(L)
		{
			const char* idstring = luaCallIDStrings[calltype];
			lua_getfield(L, LUA_REGISTRYINDEX, idstring);
			bool isFunction = lua_isfunction(L, -1);
			lua_pop(L, 1);

			if(isFunction)
			{
				keepAlive = true;
				break;
			}
		}
	}

	if(keepAlive)
	{
		if(justReturned)
		{
			if(info.print)
				info.print(uid, "script returned but is still running registered functions\r\n");
			else
				fprintf(stderr, "%s\n", "script returned but is still running registered functions");
		}
	}
	else
	{
		if(info.print)
			info.print(uid, "script finished running\r\n");
		else
			fprintf(stderr, "%s\n", "script finished running");

		StopLuaScript(uid);
	}
}

void RequestAbortLuaScript(int uid, const char* message)
{
	LuaContextInfo& info = *luaContextInfo[uid];
	lua_State* L = info.L;
	if(L)
	{
		// this probably isn't the right way to do it
		// but calling luaL_error here is positively unsafe
		// (it seemingly works fine but sometimes corrupts the emulation state in colorful ways)
		// and this works pretty well and is definitely safe, so screw it
		info.L->hookcount = 1; // run hook function as soon as possible
		info.panic = true; // and call luaL_error once we're inside the hook function
		if(message)
		{
			strncpy(info.panicMessage, message, sizeof(info.panicMessage));
			info.panicMessage[sizeof(info.panicMessage)-1] = 0;
		}
		else
		{
			strcpy(info.panicMessage, "script terminated");
		}
	}
}

void CallExitFunction(int uid)
{
	LuaContextInfo& info = *luaContextInfo[uid];
	lua_State* L = info.L;

	if(!L)
		return;

	dontworry(info);

	// first call the registered exit function if there is one
	if(!info.ranExit)
	{
		info.ranExit = true;

#ifdef USE_INFO_STACK
		infoStack.insert(infoStack.begin(), &info);
		struct Scope { ~Scope(){ infoStack.erase(infoStack.begin()); } } scope;
#endif
		lua_settop(L, 0);
		lua_getfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFOREEXIT]);
		
		if (lua_isfunction(L, -1))
		{
			bool wasRunning = info.running;
			info.running = true;
			int errorcode = lua_pcall(L, 0, 0, 0);
			info.running = wasRunning;
			if (errorcode)
			{
				info.crashed = true;
				if(L->errfunc || L->errorJmp)
					luaL_error(L, lua_tostring(L,-1));
				else if(info.print)
				{
					info.print(uid, lua_tostring(L,-1));
					info.print(uid, "\r\n");
				}
				else
				{
					fprintf(stderr, "%s\n", lua_tostring(L,-1));
				}
			}
		}
	}
}

void StopLuaScript(int uid)
{
	LuaContextInfo& info = *luaContextInfo[uid];

	if(info.running)
	{
		// if it's currently running then we can't stop it now without crashing
		// so the best we can do is politely request for it to go kill itself
		RequestAbortLuaScript(uid);
		return;
	}

	lua_State* L = info.L;
	if(L)
	{
		CallExitFunction(uid);

		if(info.onstop)
			info.onstop(uid, !info.crashed); // must happen before closing L and after the exit function, otherwise the final GUI state of the script won't be shown properly or at all

		if(info.started) // this check is necessary
		{
			lua_close(L);
#ifndef USE_INFO_STACK
			luaStateToContextMap.erase(L);
#endif
			luaStateToUIDMap.erase(L);
			info.L = NULL;
			info.started = false;
		}
		RefreshScriptStartedStatus();
	}
}

void CloseLuaContext(int uid)
{
	StopLuaScript(uid);
	delete luaContextInfo[uid];
	luaContextInfo.erase(uid);
}


void CallRegisteredLuaFunctions(LuaCallID calltype)
{
	const char* idstring = luaCallIDStrings[calltype];

	std::map<int, LuaContextInfo*>::iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo*>::iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = *iter->second;
		lua_State* L = info.L;
		if(L && (!info.panic || calltype == LUACALL_BEFOREEXIT))
		{
#ifdef USE_INFO_STACK
			infoStack.insert(infoStack.begin(), &info);
			struct Scope { ~Scope(){ infoStack.erase(infoStack.begin()); } } scope;
#endif
			// handle deferred GUI function calls and disabling deferring when unnecessary
			if(calltype == LUACALL_AFTEREMULATIONGUI || calltype == LUACALL_AFTEREMULATION)
				info.guiFuncsNeedDeferring = false;
			if(calltype == LUACALL_AFTEREMULATIONGUI)
				CallDeferredFunctions(L, deferredGUIIDString);

			lua_settop(L, 0);
			lua_getfield(L, LUA_REGISTRYINDEX, idstring);
			
			if (lua_isfunction(L, -1))
			{
				bool wasRunning = info.running;
				info.running = true;
				int errorcode = lua_pcall(L, 0, 0, 0);
				info.running = wasRunning;
				if (errorcode)
				{
					info.crashed = true;
					if(L->errfunc || L->errorJmp)
						luaL_error(L, lua_tostring(L,-1));
					else
					{
						if(info.print)
						{
							info.print(uid, lua_tostring(L,-1));
							info.print(uid, "\r\n");
						}
						else
						{
							fprintf(stderr, "%s\n", lua_tostring(L,-1));
						}
						StopLuaScript(uid);
					}
				}
			}
			else
			{
				lua_pop(L, 1);
			}

			info.guiFuncsNeedDeferring = true;
		}

		++iter;
	}
}

void CallRegisteredLuaFunctionsWithArg(LuaCallID calltype, int arg)
{
	const char* idstring = luaCallIDStrings[calltype];

	std::map<int, LuaContextInfo*>::iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo*>::iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = *iter->second;
		lua_State* L = info.L;
		if(L)
		{
#ifdef USE_INFO_STACK
			infoStack.insert(infoStack.begin(), &info);
			struct Scope { ~Scope(){ infoStack.erase(infoStack.begin()); } } scope;
#endif

			lua_settop(L, 0);
			lua_getfield(L, LUA_REGISTRYINDEX, idstring);
			
			if (lua_isfunction(L, -1))
			{
				bool wasRunning = info.running;
				info.running = true;
				lua_pushinteger(L, arg);
				int errorcode = lua_pcall(L, 1, 0, 0);
				info.running = wasRunning;
				if (errorcode)
				{
					info.crashed = true;
					if(L->errfunc || L->errorJmp)
						luaL_error(L, lua_tostring(L,-1));
					else
					{
						if(info.print)
						{
							info.print(uid, lua_tostring(L,-1));
							info.print(uid, "\r\n");
						}
						else
						{
							fprintf(stderr, "%s\n", lua_tostring(L,-1));
						}
						StopLuaScript(uid);
					}
				}
			}
			else
			{
				lua_pop(L, 1);
			}
		}

		++iter;
	}
}

void DontWorryLua() // everything's going to be OK
{
	std::map<int, LuaContextInfo*>::const_iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo*>::const_iterator end = luaContextInfo.end();
	while(iter != end)
	{
		dontworry(*iter->second);
		++iter;
	}
}

void EnableStopAllLuaScripts(bool enable)
{
	g_stopAllScriptsEnabled = enable;
}

void StopAllLuaScripts()
{
	if(!g_stopAllScriptsEnabled)
		return;

	std::map<int, LuaContextInfo*>::const_iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo*>::const_iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = *iter->second;
		bool wasStarted = info.started;
		StopLuaScript(uid);
		info.restartLater = wasStarted;
		++iter;
	}
}

void RestartAllLuaScripts()
{
	if(!g_stopAllScriptsEnabled)
		return;

	std::map<int, LuaContextInfo*>::const_iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo*>::const_iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = *iter->second;
		if(info.restartLater || info.started)
		{
			info.restartLater = false;
			RunLuaScriptFile(uid, info.lastFilename.c_str());
		}
		++iter;
	}
}

// sets anything that needs to depend on the total number of scripts running
void RefreshScriptStartedStatus()
{
	int numScriptsStarted = 0;

	std::map<int, LuaContextInfo*>::const_iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo*>::const_iterator end = luaContextInfo.end();
	while(iter != end)
	{
		LuaContextInfo& info = *iter->second;
		if(info.started)
			numScriptsStarted++;
		++iter;
	}

	frameadvSkipLagForceDisable = (numScriptsStarted != 0); // disable while scripts are running because currently lag skipping makes lua callbacks get called twice per frame advance
	g_numScriptsStarted = numScriptsStarted;
}

