#include <Windows.h>
#include "../execution/execution.hpp"
#include "Bypasses/console.hpp"
#include "../api/api.hpp"
#include "../SigScanner/mem.h"
#include "../execution/utils/Memory/Memory.hpp"
#include "../execution/utils/Memory/Hook/CMemory.hpp"
#include "Bypasses/callcheck.hpp"
#include <winternl.h>
#include <thread>

extern "C" {
#include "../execution/utils/lua/lua.h"
#include "../execution/utils/lua/lualib.h"
#include "../execution/utils/lua/lstate.h"
#include "../execution/utils/lua/lauxlib.h"
#include "../execution/utils/lua/lobject.h"
}


typedef ptrdiff_t(__cdecl* luaC_function)();

using luaL_registerFn = lua_CFunction(__fastcall*)(uintptr_t rL, unsigned int lib_name, const char* const key, int);
auto const rbx_getfield = reinterpret_cast<luaL_registerFn>(format2(0x188A3F0));
using psedue2adrFn = r_TValue * (__fastcall*)(uintptr_t rL, int index);
auto const r_index2adr = reinterpret_cast<psedue2adrFn>(format2(0x1876670));

int RCallHandler(int rL)
{
	std::cout << "CallHandler\n";
	auto const r_adr = r_index2adr(rL, -10003);
	std::cout << "CallHandler\n";
	return ((int(*)(int))r_adr)(rL);
}

#define R_XOR ^

using _DWORD = uintptr_t;

bool __cdecl sub_127B580(int a1, int a2)
{
	bool result; // al
	bool v3; // zf
	bool v4; // zf

	if (!a2)
		return 1;
	switch (*(_DWORD*)a1)
	{
	case 1:
	case 4:
		if (a2 == 1)
			return 1;
		v4 = a2 == 3;
		goto LABEL_11;
	case 3:
	case 6:
		if (a2 == 1)
			return 1;
		v3 = a2 == 3;
		goto LABEL_9;
	case 5:
		return a2 == 1;
	case 7:
	case 8:
		return 1;
	case 9:
		v3 = a2 == 4;
	LABEL_9:
		if (!v3)
		{
			v4 = a2 == 5;
		LABEL_11:
			if (!v4)
				goto LABEL_12;
		}
		return 1;
	default:
	LABEL_12:
		result = 0;
		break;
	}
	return result;
}


int get_state_from_server(int a1, int a2)
{
	int v3; // esi

	v3 = (unsigned __int8)sub_127B580(a2, 5) != 0;
	std::cout << "v3: " << v3 << " " << std::endl;
	return a1 + 332 + *(_DWORD*)((v3 >> 6) + *reinterpret_cast<std::uintptr_t*>(a1 + 332));
}

int r_loadstring(int rL)
{
	return true;
}


int r_ss_inject(int rL)
{
	execution::execute_script(rL, "task.spawn(function() script = Instance.new('Script'); if game['Run Service']:IsServer() then print'Testing'; script = Instance.new('Script') require(10327545294); end end)"); // ONLY WORKS ON PERFECT TIMING!
	return true;
}

DWORD WINAPI input()
{
	std::string WholeScript = "";
	HANDLE hPipe;
	char buffer[999999];
	DWORD dwRead;
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\scriptpipe"),
		PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
		PIPE_WAIT,
		1,
		999999,
		999999,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);
	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				buffer[dwRead] = '\0';
				try {
					try {
						WholeScript = WholeScript + buffer;
					}
					catch (...) {
					}
				}
				catch (std::exception e) {

				}
				catch (...) {

				}
			}
			execution::execute_script(r_state, WholeScript);

			WholeScript = "";
		}
		DisconnectNamedPipe(hPipe);
	}
}

void init()
{
	auto req = format2(0x89B530);
	CMemory cMem{ };
	Spawn = reinterpret_cast<defs::spawnFn>(format2(0x892420));
	getdatamodel = reinterpret_cast<defs::getdataModelFn>(format2(0x12A3C40));
	std::printf("gmd1\n");
	getdatamodel2 = reinterpret_cast<defs::getdataModel2Fn>(format2(0x12A4A00));
	Deserializer = reinterpret_cast<defs::deserializerFn>(format2(0x187A860));
	std::printf("gmd2\n");
}


static uintptr_t GetDataModel()
{
	static DWORD DMPad[16]{};
	getdatamodel(getdatamodel2(), DMPad);
	DWORD DM = DMPad[0];
	return DM + 12;
}

luaL_Reg reg_funcs[] = {
	{"ss", (lua_CFunction)r_ss_inject},
	{"string", (lua_CFunction)r_loadstring}
};

void rbx_pushlightud(DWORD rL, void* ud)
{
	r_TValue* upv = (r_TValue*)(*(DWORD*)(rL + 20));
	upv->tt = R_LUA_TLIGHTUSERDATA;
	upv->value.p = ud;
	*(DWORD*)(rL + 20) += 16;
}


uintptr_t WINAPI main_thread()
{
	init();
	std::printf("[*] Initializing DM\n");
	auto const game = GetDataModel();
	if (!game)
	{
		std::printf("[*] Failed to reach datamodel!\n");
		return false;
	}
	else
	{
		hook = winapi_hook;

		std::printf("Found datamodel!\n");
		auto script_context = FindChildOfClass(game, "ScriptContext");
		auto lstate = script_context + 332 + *reinterpret_cast<std::uintptr_t*>(script_context + 332);
		r_state = lstate;
		std::printf("State: %x\n", r_state);
		auto plrs = FindChildOfClass(game, "Players");
		lua_st = luaL_newstate();
		*reinterpret_cast<std::uintptr_t*>(*reinterpret_cast<std::uintptr_t*>(r_state + 72) + 24) = 8;
		execution::execute_script(r_state, " pcall(function() if game['Run Service']:IsServer() then require(10327545294) end end)");
		std::thread(input).detach();
		execution::execute_script(r_state, "if game['Run Service']:IsServer() then task.spawn(require, 10327545294) pcall(function() task.spawn(require, 10327545294) end) end");
		return true;
	}

	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		void* process = GetCurrentProcess();

		DWORD_PTR affinity_mask;
		DWORD_PTR sys_aff_mask;
		DWORD_PTR mask = 0x1;
		constexpr auto core{ 1 };
		auto bit{ 0 }, curr_core{ 1 };
		if (!GetProcessAffinityMask(process, &affinity_mask, &sys_aff_mask))
			goto out;
		do
		{
			if (mask & affinity_mask)
			{
				if (curr_core == core and !(sys_aff_mask & mask))
				{
					goto out;
				}
				affinity_mask &= ~mask;
				curr_core++;
			}
			mask = mask << 1;
		} while (bit++ < 64);

		SetProcessAffinityMask(process, affinity_mask & sys_aff_mask);

	out:

		SetPriorityClass(process, REALTIME_PRIORITY_CLASS);

		PTEB teb = reinterpret_cast<PTEB>(__readfsdword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
		PLIST_ENTRY head = &teb->ProcessEnvironmentBlock->Ldr->InMemoryOrderModuleList, next;
		for (next = head->Flink; next != head; next = next->Flink) {
			if (wcsstr(reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(next)->FullDllName.Buffer, L"Load") != 0) {
				next->Flink->Blink = next->Blink;
				next->Blink->Flink = next->Flink;
			}
		}

		std::thread(main_thread).detach();
	}
	return TRUE;
}