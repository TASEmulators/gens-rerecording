#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

void OpenLuaContext(int uid, void(*print)(int uid, const char* str));
void RunLuaScriptFile(int uid, const char* filename);
void StopLuaScript(int uid);
void RequestAbortLuaScript(int uid);
void CloseLuaContext(int uid);

enum LuaCallID
{
	LUACALL_BEFOREEMULATION,
	LUACALL_AFTEREMULATION,
	LUACALL_AFTEREMULATIONGUI,
	LUACALL_BEFOREEXIT,
	LUACALL_BEFORESAVE,
	LUACALL_AFTERLOAD,

	LUACALL_COUNT
};
void CallRegisteredLuaFunctions(LuaCallID calltype);
void CallRegisteredLuaFunctionsWithArg(LuaCallID calltype, int arg);
void StopAllLuaScripts();
void DontWorryLua();

#ifdef __cplusplus
}
#endif

#endif

