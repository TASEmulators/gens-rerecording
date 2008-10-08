#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

void OpenLuaContext(int uid, void(*print)(int uid, const char* str) = 0, void(*onstart)(int uid) = 0, void(*onstop)(int uid) = 0);
void RunLuaScriptFile(int uid, const char* filename);
void StopLuaScript(int uid);
void RequestAbortLuaScript(int uid, const char* message = 0);
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
void RestartAllLuaScripts();
void DontWorryLua();

#ifdef __cplusplus
}
#endif

#endif

