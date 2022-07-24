#pragma once
#include "../Win32/Rblx/DataModel.hpp"
#include "bytecode_serializer/serializer.hpp"

static std::uintptr_t r_state = 0;
static lua_State* lua_st;
#define rlua_pop(state,n)            r_lua_settop(state, -(n)-1)

typedef union r_GCObject r_GCObject;

struct r_GCheader
{
	r_GCObject* next;
	byte marked;
	byte tt;
};

union r_GCObject
{
	r_GCheader gch;
	DWORD ts; /* tstring */
	DWORD u; /* userdata */
	DWORD cl; /* closure */
	DWORD h; /* table */
	DWORD p; /* pointer */
	DWORD uv; /* upvalue */
	DWORD th;  /* thread */
};

typedef union
{
	r_GCObject* gc;
	void* p;
	double n;
	int b;
} r_Value;

#define r_TValuefields    r_Value value; union { r_lua_TValue *upv; int flg; }; int tt

typedef struct r_lua_TValue
{
	r_TValuefields;
} r_TValue;

namespace execution
{
	void execute_script(std::uintptr_t r_state, const std::string& script);
}

inline FORCEINLINE void r_lua_settop(std::uintptr_t state, int index)
{
	*(r_TValue**)(state + 20) += index + 1;
}

void execution::execute_script(std::uintptr_t r_state, const std::string& script)
{
	auto bytecode = serialize(lua_st, script);
	DWORD oldprotect;
	uintptr_t retcheck_func = r_base + 0x02D50F0;
	VirtualProtect((LPVOID)retcheck_func, 1, PAGE_EXECUTE_READWRITE, &oldprotect);
	byte oldbyte = *(byte*)(retcheck_func);
	*(byte*)(retcheck_func) = 0xC3;
	auto des = Deserializer(r_state, "", bytecode.c_str(), bytecode.size(), false);
	std::printf("des: %i\n", des);
	Spawn(r_state);
	*(byte*)(retcheck_func) = oldbyte;
	VirtualProtect((LPVOID)retcheck_func, 1, oldprotect, &oldprotect);
	rlua_pop(r_state, 1);
}