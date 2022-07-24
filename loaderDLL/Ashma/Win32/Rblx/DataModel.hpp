#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#define format2(address) (address - 0x00400000 + reinterpret_cast<uintptr_t>(GetModuleHandle(0)))
static uintptr_t r_base = reinterpret_cast<uintptr_t>(GetModuleHandleA(0));


#define R_LUA_TNIL 0
#define R_LUA_TBOOLEAN 1
#define R_LUA_TVECTOR 2
#define R_LUA_TLIGHTUSERDATA 3
#define R_LUA_TNUMBER 4
#define R_LUA_TSTRING 5
#define R_LUA_TUSERDATA 6
#define R_LUA_TTHREAD 7
#define R_LUA_TFUNCTION 8
#define R_LUA_TTABLE 9

#if true
#define Declare(address, returnValue, callingConvention, ...) (returnValue(callingConvention*)(__VA_ARGS__))(format2(address))
#else
#define Declare(address, returnValue, callingConvention, ...) (returnValue(callingConvention*)(__VA_ARGS__))(address)
#endif 

static std::vector<std::uintptr_t> GetChildren(const std::uintptr_t instance)
{
	std::vector<std::uintptr_t> children;

	if (!instance)
		return children;

	const auto current = *reinterpret_cast<const std::uintptr_t*>(instance + 40);
	if (!current) return children;

	const auto end = *reinterpret_cast<const std::uintptr_t*>(current + 4u);

	if ((!end) || (end < 0))
	{
		return children;
	}

	for (auto parent = *reinterpret_cast<const std::uintptr_t*>(current); parent < end; parent += 8u)
		children.push_back(*reinterpret_cast<const std::uintptr_t*>(parent));

	return children;
}

static std::string GetClassType(std::uintptr_t Name)
{
	if (!Name)
		return "";

	std::string CurrentName;

	DWORD ClassDescriptor = *reinterpret_cast<std::uintptr_t*>(Name + 0xC);
	CurrentName = *reinterpret_cast<std::string*>(*reinterpret_cast<std::uintptr_t*>((ClassDescriptor + 0x4)));

	return CurrentName;
}

static std::uintptr_t FindChildOfClass(DWORD Instance, std::string Class)
{
	if (!Instance)
		return NULL;

	for (std::uintptr_t Child : GetChildren(Instance))
	{
		if (GetClassType(Child) == Class) {
			return Child;
		}
	}
}

static std::uintptr_t GetParent(std::uintptr_t Instance)
{
	return *reinterpret_cast<std::uintptr_t*>(Instance + 0x30);
}


namespace defs
{
	using deserializerFn = void* (__fastcall*)(std::uintptr_t rL, const char* chunk, const char* bc, std::size_t size, bool Enviroment);
	using getdataModelFn = ptrdiff_t(__thiscall*)(std::uintptr_t instance, DWORD* nothing_chunk);
	using getdataModel2Fn = ptrdiff_t(__cdecl*)();

	using spawnFn = void* (__cdecl*)(std::uintptr_t rL);
	using get_lua_stateFn = std::uintptr_t(__thiscall*)(uintptr_t script_context, int index);
}

static defs::deserializerFn Deserializer;
static defs::spawnFn Spawn;
static defs::getdataModel2Fn getdatamodel2;
static defs::getdataModelFn getdatamodel;
static defs::get_lua_stateFn get_state_index;
