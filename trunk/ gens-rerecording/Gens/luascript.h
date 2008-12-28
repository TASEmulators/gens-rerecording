#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H


void OpenLuaContext(int uid, void(*print)(int uid, const char* str) = 0, void(*onstart)(int uid) = 0, void(*onstop)(int uid, bool statusOK) = 0);
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

struct LuaSaveData
{
	LuaSaveData() { recordList = 0; }
	~LuaSaveData() { ClearRecords(); }

	struct Record
	{
		unsigned int key; // crc32
		unsigned int size; // size of data
		unsigned char* data;
		Record* next;
	};

	Record* recordList;

	void SaveRecord(int uid, unsigned int key); // saves Lua stack into a record and pops it
	void LoadRecord(int uid, unsigned int key) const; // pushes a record's data onto the Lua stack

	void ExportRecords(void* file) const; // writes all records to an already-open file
	void ImportRecords(void* file); // reads records from an already-open file
	void ClearRecords(); // deletes all record data

private:
	// disallowed, it's dangerous to call this
	// (because the memory the destructor deletes isn't refcounted and shouldn't need to be copied)
	// so pass LuaSaveDatas by reference and this should never get called
	LuaSaveData(const LuaSaveData& copy) {}
};
void CallRegisteredLuaSaveFunctions(int savestateNumber, LuaSaveData& saveData);
void CallRegisteredLuaLoadFunctions(int savestateNumber, const LuaSaveData& saveData);

void StopAllLuaScripts();
void RestartAllLuaScripts();
void EnableStopAllLuaScripts(bool enable);
void DontWorryLua();



#endif

