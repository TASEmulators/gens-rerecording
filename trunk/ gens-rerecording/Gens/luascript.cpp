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

extern "C" {
	#include "lua/src/lua.h"
	#include "lua/src/lauxlib.h"
	#include "lua/src/lualib.h"
	#include "lua/src/lstate.h"
};

struct LuaContextInfo {
	bool started;
	bool running;
	bool restart;
	bool restartLater;
	lua_State* L;
	int worryCount;
	bool panic;
	bool ranExit;
	int speedMode;
	char panicMessage [64];
	std::string lastFilename;
	void(*print)(int uid, const char* str);
	void(*onstart)(int uid);
	void(*onstop)(int uid);
};
std::map<int, LuaContextInfo> luaContextInfo;
std::map<lua_State*, int> luaStateToContextMap; // because callbacks from lua will only give us a Lua_State to work with

void worry(lua_State* L, int intensity)
{
	int uid = luaStateToContextMap[L];
	LuaContextInfo& info = luaContextInfo[uid];
	if(info.worryCount >= 0)
		info.worryCount += intensity;
}

static int print(lua_State* L)
{
	int uid = luaStateToContextMap[L];
	LuaContextInfo& info = luaContextInfo[uid];

	char str[1024];
	str[0] = 0;
	char* ptr = str;

	int n=lua_gettop(L);
	int i;
	for (i=1; i<=n; i++)
	{
		//if (i>1)
		//	ptr += sprintf(ptr, "\t");
		if (lua_isstring(L,i))
			ptr += sprintf(ptr, "%s",lua_tostring(L,i));
		else if (lua_isnil(L,i))
			ptr += sprintf(ptr, "%s","nil");
		else if (lua_isboolean(L,i))
			ptr += sprintf(ptr, "%s",lua_toboolean(L,i) ? "true" : "false");
		else
			ptr += sprintf(ptr, "%s:%p",luaL_typename(L,i),lua_topointer(L,i));
	}
	ptr += sprintf(ptr, "\r\n");

	if(info.print)
		info.print(uid, str);
	else
		puts(str);

	worry(L, 10);
	return 0;
}

#define HOOKCOUNT 4096
#define MAX_WORRY_COUNT 600
bool rescueDisabled = false;
int numTries = 0;
void LuaRescueHook(lua_State* L, lua_Debug *dbg)
{
	int uid = luaStateToContextMap[L];
	LuaContextInfo& info = luaContextInfo[uid];

	if(info.worryCount < 0 && !info.panic)
		return;

	info.worryCount++;

	if(info.worryCount > MAX_WORRY_COUNT || info.panic)
	{
		info.worryCount = 0;

		int answer;
		if(info.panic)
		{
			answer = IDYES;
		}
		else
		{
			DialogsOpen++;
			answer = MessageBox(HWnd, "A Lua script has been running for quite a while. Maybe it is in an infinite loop.\n\nWould you like to stop the script?\n\n(Yes to stop it now,\n No to keep running and not ask again,\n Cancel to keep running but ask again later)", "Lua Alert", MB_YESNOCANCEL | MB_DEFBUTTON3 | MB_ICONASTERISK);
			DialogsOpen--;
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

int emulateframe(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	Update_Emulation_One(HWnd);

	worry(L,30);
	return 0;
}

int emulateframefastnoskipping(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	Update_Emulation_One_Before(HWnd);
	Update_Frame_Hook();
	Update_Emulation_After_Controlled(HWnd, true);

	worry(L,20);
	return 0;
}

int emulateframefast(lua_State* L)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started)))
		return 0;

	disableVideoLatencyCompensationCount = VideoLatencyCompensation + 1;

	Update_Emulation_One_Before(HWnd);

	if(FrameCount%16 == 0)
	{
		Update_Frame_Hook();
		Update_Emulation_After_Controlled(HWnd, true);
	}
	else
	{
		Update_Frame_Fast_Hook();
		Update_Emulation_After_Controlled(HWnd, false);
	}

	worry(L,15);
	return 0;
}

int emulateframeinvisible(lua_State* L)
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

	worry(L,10);
	return 0;
}

int speedmode(lua_State* L)
{
	int newSpeedMode = 0;
	if(lua_isnumber(L,1))
		newSpeedMode = luaL_checkinteger(L,1);
	else
	{
		const char* str = luaL_checkstring(L,1);
		if(!stricmp(str, "normal"))
			newSpeedMode = 0;
		else if(!stricmp(str, "nothrottle"))
			newSpeedMode = 1;
		else if(!stricmp(str, "turbo"))
			newSpeedMode = 2;
		else if(!stricmp(str, "maximum"))
			newSpeedMode = 3;
	}

	int uid = luaStateToContextMap[L];
	LuaContextInfo& info = luaContextInfo[uid];
	info.speedMode = newSpeedMode;
	return 0;
}

int frameadvance(lua_State* L)
{
	int uid = luaStateToContextMap[L];
	LuaContextInfo& info = luaContextInfo[uid];
	switch(info.speedMode)
	{
		default:
		case 0: // "normal"
			while(!Step_Gens_MainLoop(true, true));
			break;
		case 1: // "nothrottle"
			while(!Step_Gens_MainLoop(Paused!=0, false));
			if(!(FastForwardKeyDown && (GetActiveWindow()==HWnd || BackgroundInput)))
				emulateframefastnoskipping(L);
			else
				emulateframefast(L);
			break;
		case 2: // "turbo"
			while(!Step_Gens_MainLoop(Paused!=0, false));
			emulateframefast(L);
			break;
		case 3: // "maximum"
			while(!Step_Gens_MainLoop(Paused!=0, false));
			emulateframeinvisible(L);
			break;
	}
	return 0;
}

static const char* luaCallIDStrings [] =
{
	"_BEFOREEMULATION",
	"_AFTEREMULATION",
	"_AFTEREMULATIONGUI",
	"_BEFOREEXIT",
	"_BEFORESAVE",
	"_AFTERLOAD",
};
static const int _makeSureWeHaveTheRightNumberOfStrings [sizeof(luaCallIDStrings)/sizeof(*luaCallIDStrings) == LUACALL_COUNT ? 1 : 0];

int registerbefore(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFOREEMULATION]);
	return 0;
}
int registerafter(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_AFTEREMULATION]);
	return 0;
}
int registerexit(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFOREEXIT]);
	return 0;
}
int registergui(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_AFTEREMULATIONGUI]);
	return 0;
}
int registersave(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_BEFORESAVE]);
	return 0;
}
int registerload(lua_State* L)
{
	if (!lua_isnil(L,1))
		luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[LUACALL_AFTERLOAD]);
	return 0;
}


int readbyte(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned char value = (unsigned char)(ReadValueAtHardwareAddress(address, 1) & 0xFF);
	lua_pushinteger(L, value);
	return 1; // we return the number of return values
}
int readbytesigned(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	signed char value = (signed char)(ReadValueAtHardwareAddress(address, 1) & 0xFF);
	lua_pushinteger(L, value);
	return 1;
}
int readword(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned short value = (unsigned short)(ReadValueAtHardwareAddress(address, 2) & 0xFFFF);
	lua_pushinteger(L, value);
	return 1;
}
int readwordsigned(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	signed short value = (signed short)(ReadValueAtHardwareAddress(address, 2) & 0xFFFF);
	lua_pushinteger(L, value);
	return 1;
}
int readlong(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned long value = (unsigned long)(ReadValueAtHardwareAddress(address, 4));
	lua_pushinteger(L, value);
	return 1;
}
int readlongsigned(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	signed long value = (signed long)(ReadValueAtHardwareAddress(address, 4));
	lua_pushinteger(L, value);
	return 1;
}

int writebyte(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned char value = (unsigned char)(luaL_checkinteger(L,2) & 0xFF);
	WriteValueAtHardwareAdress(address, value, 1);
	return 0;
}
int writeword(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned short value = (unsigned short)(luaL_checkinteger(L,2) & 0xFFFF);
	WriteValueAtHardwareAdress(address, value, 2);
	return 0;
}
int writelong(lua_State* L)
{
	int address = luaL_checkinteger(L,1);
	unsigned short value = (unsigned short)(luaL_checkinteger(L,2));
	WriteValueAtHardwareAdress(address, value, 4);
	return 0;
}

int statecreate(lua_State* L)
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
int stateget(lua_State* L)
{
	void* stateBuffer = lua_touserdata(L,1);

	if(stateBuffer)
		Save_State_To_Buffer((unsigned char*)stateBuffer);

	return 1;
}
int stateset(lua_State* L)
{
	void* stateBuffer = lua_touserdata(L,1);

	if(stateBuffer)
		Load_State_From_Buffer((unsigned char*)stateBuffer);

	return 0;
}
int statesave(lua_State* L)
{
	int stateNumber = luaL_checkinteger(L,1);
	Set_Current_State(stateNumber);
	Str_Tmp[0] = 0;
	Get_State_File_Name(Str_Tmp);
	Save_State(Str_Tmp);
	return 0;
}
int stateload(lua_State* L)
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
	{1, 0, "Up"},
	{1, 1, "Down"},
	{1, 2, "Left"},
	{1, 3, "Right"},
	{1, 4, "A"},
	{1, 5, "B"},
	{1, 6, "C"},
	{1, 7, "Start"},
	{1, 32, "X"},
	{1, 33, "Y"},
	{1, 34, "Z"},
	{1, 35, "Mode"},
	{2, 24, "Up"},
	{2, 25, "Down"},
	{2, 26, "Left"},
	{2, 27, "Right"},
	{2, 28, "A"},
	{2, 29, "B"},
	{2, 30, "C"},
	{2, 31, "Start"},
	{2, 36, "X"},
	{2, 37, "Y"},
	{2, 38, "Z"},
	{2, 39, "Mode"},
};


int joyset(lua_State* L)
{
	int controllerNumber = luaL_checkinteger(L,1);

	luaL_checktype(L, 2, LUA_TTABLE);

	int input = ~0;
	int mask = 0;

	for(int i = 0; i < sizeof(s_buttonDescs)/sizeof(*s_buttonDescs); i++)
	{
		const ButtonDesc& bd = s_buttonDescs[i];
		if(bd.controllerNum == controllerNumber)
		{
			lua_getfield(L, 2, bd.name);
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
int joyget(lua_State* L)
{
	int controllerNumber = luaL_checkinteger(L,1);
	lua_newtable(L);

	long long input = GetCurrentInputCondensed();

	for(int i = 0; i < sizeof(s_buttonDescs)/sizeof(*s_buttonDescs); i++)
	{
		const ButtonDesc& bd = s_buttonDescs[i];
		if(bd.controllerNum == controllerNumber)
		{
			bool pressed = (input & ((long long)1<<bd.bit)) == 0;
			lua_pushboolean(L, pressed);
			lua_setfield(L, -2, bd.name);
		}
	}

	return 1;
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

int getcolor(lua_State *L, int idx, int defaultColor)
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
	}
	return defaultColor;
}

int guitext(lua_State* L)
{
	int x = luaL_checkinteger(L,1) & 0xFFFF;
	int y = luaL_checkinteger(L,2) & 0xFFFF;
	const char* str = luaL_checkstring(L,3);
	if(str && *str)
	{
		int foreColor = getcolor(L,4,0xFFFFFFFF);
		int backColor = getcolor(L,5,0x000000FF);
		PutText2(str, x, y, foreColor, backColor);
	}
	return 0;
}
int guibox(lua_State* L)
{
	int x1 = luaL_checkinteger(L,1) & 0xFFFF;
	int y1 = luaL_checkinteger(L,2) & 0xFFFF;
	int x2 = luaL_checkinteger(L,3) & 0xFFFF;
	int y2 = luaL_checkinteger(L,4) & 0xFFFF;
	int fillcolor = getcolor(L,5,0xFFFFFF3F);
	int outlinecolor = getcolor(L,6,0xFFFFFFFF);

	DrawBoxPP2 (x1, y1, x2, y2, fillcolor, outlinecolor);

	return 0;
}
int guipixel(lua_State* L)
{
	int x = luaL_checkinteger(L,1) & 0xFFFF;
	int y = luaL_checkinteger(L,2) & 0xFFFF;
	int color = getcolor(L,3,0xFFFFFFFF);
	int color32 = color>>8;
	int color16 = DrawUtil::Pix32To16(color32);
	int Opac = color & 0xFF;

	Pixel(x, y, color32, color16, 0, Opac);

	return 0;
}
int guiline(lua_State* L)
{
	int x1 = luaL_checkinteger(L,1) & 0xFFFF;
	int y1 = luaL_checkinteger(L,2) & 0xFFFF;
	int x2 = luaL_checkinteger(L,3) & 0xFFFF;
	int y2 = luaL_checkinteger(L,4) & 0xFFFF;
	int color = getcolor(L,5,0xFFFFFFFF);
	int color32 = color>>8;
	int color16 = DrawUtil::Pix32To16(color32);
	int Opac = color & 0xFF;

	DrawLine(x1, y1, x2, y2, color32, color16, 0, Opac);

	return 0;
}

int getframecount(lua_State* L)
{
	lua_pushinteger(L, FrameCount);
	return 1;
}
int getlagcount(lua_State* L)
{
	lua_pushinteger(L, LagCountPersistent);
	return 1;
}
int getmovielength(lua_State* L)
{
	lua_pushinteger(L, MainMovie.LastFrame);
	return 1;
}
int ismovieactive(lua_State* L)
{
	lua_pushboolean(L, MainMovie.File != NULL);
	return 1;
}
int getrerecordcount(lua_State* L)
{
	lua_pushinteger(L, MainMovie.NbRerecords);
	return 1;
}
int getreadonly(lua_State* L)
{
	lua_pushboolean(L, MainMovie.ReadOnly);
	return 1;
}
int setreadonly(lua_State* L)
{
	bool readonly = lua_toboolean(L,1) != 0;
	MainMovie.ReadOnly = readonly;
	return 0;
}
int ismovierecording(lua_State* L)
{
	lua_pushboolean(L, MainMovie.Status == MOVIE_RECORDING);
	return 1;
}
int ismovieplaying(lua_State* L)
{
	lua_pushboolean(L, MainMovie.Status == MOVIE_PLAYING);
	return 1;
}
int getmoviename(lua_State* L)
{
	lua_pushstring(L, MainMovie.FileName);
	return 1;
}

int clearaudio(lua_State* L)
{
	Clear_Sound_Buffer();
	return 0;
}

// resets our "worry" counter of the Lua state
int dontworry(lua_State* L)
{
	if(!L)
		return 0;
	int uid = luaStateToContextMap[L];
	LuaContextInfo& info = luaContextInfo[uid];
	info.worryCount = min(info.worryCount, 0);
	return 0;
}

static const struct luaL_reg genslib [] =
{
	{"frameadvance", frameadvance},
	{"speedmode", speedmode},
	{"emulateframe", emulateframe},
	{"emulateframefastnoskipping", emulateframefastnoskipping},
	{"emulateframefast", emulateframefast},
	{"emulateframeinvisible", emulateframeinvisible},
	{"framecount", getframecount},
	{"lagcount", getlagcount},
	{"registerbefore", registerbefore},
	{"registerafter", registerafter},
	{"registerexit", registerexit},
	{"dontworry", dontworry},
	//{"message", message},
	{NULL, NULL}
};
static const struct luaL_reg guilib [] =
{
	{"register", registergui},
	{"text", guitext},
	{"drawtext", guitext},
	{"box", guibox},
	{"drawbox", guibox},
	{"pixel", guipixel},
	{"drawpixel", guipixel},
	{"line", guiline},
	{"drawline", guiline},
	{NULL, NULL}
};
static const struct luaL_reg statelib [] =
{
	{"create", statecreate},
	{"set", stateset},
	{"get", stateget},
	{"save", statesave},
	{"load", stateload},
	{"registersave", registersave},
	{"registerload", registerload},
	{NULL, NULL}
};
static const struct luaL_reg memorylib [] =
{
	{"readbyte", readbyte},
	{"readbyteunsigned", readbyte},
	{"readbytesigned", readbytesigned},
	{"readword", readword},
	{"readwordunsigned", readword},
	{"readwordsigned", readwordsigned},
	{"readlong", readlong},
	{"readlongunsigned", readlong},
	{"readlongsigned", readlongsigned},
	{"writebyte", writebyte},
	{"writeword", writeword},
	{"writelong", writelong},
	{NULL, NULL}
};
static const struct luaL_reg joylib [] =
{
	{"get", joyget},
	{"set", joyset},
	{"read", joyget},
	{"write", joyset},
	{NULL, NULL}
};
static const struct luaL_reg movielib [] =
{
	{"length", getmovielength},
	{"active", ismovieactive},
	{"recording", ismovierecording},
	{"playing", ismovieplaying},
	{"name", getmoviename},
	{"rerecordcount", getrerecordcount},
	{"readonly", getreadonly},
	{"getreadonly", getreadonly},
	{"setreadonly", setreadonly},
	{NULL, NULL}
};
static const struct luaL_reg soundlib [] =
{
	{"clear", clearaudio},
	{NULL, NULL}
};

void OpenLuaContext(int uid, void(*print)(int uid, const char* str), void(*onstart)(int uid), void(*onstop)(int uid))
{
	LuaContextInfo newInfo;
	newInfo.started = false;
	newInfo.running = false;
	newInfo.restart = false;
	newInfo.restartLater = false;
	newInfo.worryCount = 0;
	newInfo.panic = false;
	newInfo.ranExit = false;
	newInfo.speedMode = 0;
	newInfo.print = print;
	newInfo.onstart = onstart;
	newInfo.onstop = onstop;
	newInfo.L = NULL;
	luaContextInfo[uid] = newInfo;
}

void RunLuaScriptFile(int uid, const char* filenameCStr)
{
	StopLuaScript(uid);

	LuaContextInfo& info = luaContextInfo[uid];

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
		luaStateToContextMap[L] = uid;
		info.L = L;
		info.worryCount = 0;
		info.panic = false;
		info.ranExit = false;
		info.restart = false;
		info.restartLater = false;
		info.speedMode = 0;
		info.lastFilename = filename;

		luaL_openlibs(L);
		luaL_register(L, "gens", genslib);
		luaL_register(L, "gui", guilib);
		luaL_register(L, "state", statelib); // I like this name more
		luaL_register(L, "savestate", statelib); // but others might be more familiar with this one
		luaL_register(L, "memory", memorylib);
		luaL_register(L, "joypad", joylib);
		luaL_register(L, "movie", movielib);
		luaL_register(L, "sound", soundlib);
		lua_register(L, "print", print);

		// register a function to periodically check for inactivity
		lua_sethook(L, LuaRescueHook, LUA_MASKCOUNT, HOOKCOUNT);

		info.started = true;
		if(info.onstart)
			info.onstart(uid);
		info.running = true;
		int errorcode = luaL_dofile(L,filename.c_str());
		info.running = false;

		if (errorcode)
		{
			if(info.print)
			{
				info.print(uid, lua_tostring(L,-1));
				info.print(uid, "\r\n");
			}
			else
			{
				fprintf(stderr, "%s\r\n", lua_tostring(L,-1));
			}
			StopLuaScript(uid);
		}
	} while(info.restart);
}

void RequestAbortLuaScript(int uid, const char* message)
{
	LuaContextInfo& info = luaContextInfo[uid];
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
	LuaContextInfo& info = luaContextInfo[uid];
	lua_State* L = info.L;

	if(!L)
		return;

	dontworry(L);

	// first call the registered exit function if there is one
	if(!info.ranExit)
	{
		info.ranExit = true;

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
				if(L->errfunc || L->errorJmp)
					luaL_error(L, lua_tostring(L,-1));
				else if(info.print)
				{
					info.print(uid, lua_tostring(L,-1));
					info.print(uid, "\r\n");
				}
				else
				{
					fprintf(stderr, "%s\r\n", lua_tostring(L,-1));
				}
			}
		}
	}
}

void StopLuaScript(int uid)
{
	LuaContextInfo& info = luaContextInfo[uid];

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

		lua_close(L);
		luaStateToContextMap.erase(L);
		info.L = NULL;
		info.started = false;
		if(info.onstop)
			info.onstop(uid);
	}
}

void CloseLuaContext(int uid)
{
	StopLuaScript(uid);
	luaContextInfo.erase(uid);
}


void CallRegisteredLuaFunctions(LuaCallID calltype)
{
	const char* idstring = luaCallIDStrings[calltype];

	std::map<int, LuaContextInfo>::iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo>::iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = iter->second;
		lua_State* L = info.L;
		if(L)
		{
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
							fprintf(stderr, "%s\r\n", lua_tostring(L,-1));
						}
						StopLuaScript(uid);
					}
				}
			}
		}

		++iter;
	}
}

void CallRegisteredLuaFunctionsWithArg(LuaCallID calltype, int arg)
{
	const char* idstring = luaCallIDStrings[calltype];

	std::map<int, LuaContextInfo>::iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo>::iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = iter->second;
		lua_State* L = info.L;
		if(L)
		{
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
							fprintf(stderr, "%s\r\n", lua_tostring(L,-1));
						}
						StopLuaScript(uid);
					}
				}
			}
		}

		++iter;
	}
}

void DontWorryLua() // everything's going to be OK
{
	std::map<lua_State*, int>::const_iterator iter = luaStateToContextMap.begin();
	std::map<lua_State*, int>::const_iterator end = luaStateToContextMap.end();
	while(iter != end)
	{
		lua_State* L = iter->first;
		dontworry(L);
		++iter;
	}
}

void StopAllLuaScripts()
{
	std::map<int, LuaContextInfo>::const_iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo>::const_iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = luaContextInfo[uid];
		bool wasStarted = info.started;
		StopLuaScript(uid);
		info.restartLater = wasStarted;
		++iter;
	}
}

void RestartAllLuaScripts()
{
	std::map<int, LuaContextInfo>::const_iterator iter = luaContextInfo.begin();
	std::map<int, LuaContextInfo>::const_iterator end = luaContextInfo.end();
	while(iter != end)
	{
		int uid = iter->first;
		LuaContextInfo& info = luaContextInfo[uid];
		if(info.restartLater || info.started)
		{
			info.restartLater = false;
			RunLuaScriptFile(uid, info.lastFilename.c_str());
		}
		++iter;
	}
}
