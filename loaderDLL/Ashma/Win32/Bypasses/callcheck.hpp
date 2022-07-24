#pragma once
#include <Windows.h>
#include <exception>
#include <string>
#include "MinHook.h"
#pragma comment (lib, "libMinHook-x86-v110-mt.lib")


/* addressess are addressess to a call instruction and instruction right after it
* a call instruction to that winapi function
* basically routing the C function through a call instruction in range
* then to a winapi function i control
* idea by brandon
*
* trustcheck bypass is accomplished because roblox is retarded
* they throw an exception if trustcheck failed, we simply hook exception
*/

#define IdaBase1 0x400000
#define RobloxBase1 (int)GetModuleHandle(NULL)
#define RebaseAddress1(x) (RobloxBase1 + x - IdaBase1)

#define CALLCHECK_PRE_ADDR 0x64BF90 
#define CALLCHECK_POST_ADDR 0x64BF90

DWORD winapi_hook = RebaseAddress1(CALLCHECK_PRE_ADDR);
DWORD winapi_retaddr;
DWORD func_hook;
DWORD jmp_back;
int hook;

typedef void(WINAPI* CreateEventFn)(LPSECURITY_ATTRIBUTES lp_event_attributes, BOOL b_banual_reset, BOOL b_initial_state, LPCTSTR lp_name);
CreateEventFn create_eventa = NULL;

typedef void(WINAPI* RaiseExceptionNew)(DWORD dw_exceptioncode, DWORD dw_exceptionflags, DWORD nNumberOfArguments, CONST ULONG_PTR* lpArguments);
RaiseExceptionNew aRaiseException = NULL;

void WINAPI raiseexception_hook(DWORD dw_exceptioncode, DWORD dw_exceptionflags, DWORD nNumberOfArguments, const ULONG_PTR* lpArguments)
{
	try
	{
		if (dw_exceptioncode == 0xE06D7363 && nNumberOfArguments >= 3)
		{
			struct _CatchableType
			{
				unsigned int  properties;
				int     thisDisplacement[3];
				int     sizeOrOffset;
				void* copyFunction;
			};

			struct _CatchableTypeArray
			{
				int     nCatchableTypes;
				_CatchableType* arrayOfCatchableTypes[];
			};

			struct _ThrowInfo
			{
				unsigned int          attributes;
				void* pmfnUnwind;
				int(__cdecl* pForwardCompat)(...);
				_CatchableTypeArray* pCatchableTypeArray;
			};

			_ThrowInfo* throw_info = (_ThrowInfo*)lpArguments[2];
			_CatchableTypeArray* list = &throw_info->pCatchableTypeArray[0];
			_CatchableType* type = list->arrayOfCatchableTypes[list->nCatchableTypes - 1];

		
		}
	}
	catch (std::exception) {}
	aRaiseException(dw_exceptioncode, dw_exceptionflags, nNumberOfArguments, lpArguments);
}

__declspec(naked) void createeventa_hook()
{
	__asm
	{
		mov eax, [esp]
		cmp eax, winapi_retaddr
		jne normal_case
		add esp, 4
		push[esp + 4]
		call func_hook
		add esp, 4
		retn
		normal_case :
		jmp[jmp_back]
	}
}

void create_event_hook(DWORD hook_address)
{

	//Setup Addresses
	func_hook = hook_address;
	winapi_retaddr = RebaseAddress1(CALLCHECK_POST_ADDR);

	//Create Hook
	if (MH_CreateHook(GetProcAddress(GetModuleHandleA("KERNELBASE.dll"), "CreateEventA"), &createeventa_hook, reinterpret_cast<LPVOID*>(&create_eventa)) != MH_OK)
	{
		MessageBoxA(NULL, "ERROR: Failed to init bypass!", "Ashma", MB_OK);
		exit(0);
	}

	if (MH_EnableHook(GetProcAddress(GetModuleHandleA("KERNELBASE.dll"), "CreateEventA")) != MH_OK)
	{
		MessageBoxA(NULL, "ERROR: Failed to init bypass!", "Ashma", MB_OK);
		exit(0);
	}

	jmp_back = (DWORD)create_eventa;

}

DWORD winapi_returnaddr;
DWORD function_hook;
DWORD jump_back;

__declspec(naked) void createexceptiona_hook()
{
	__asm
	{
		mov eax, [esp]
		cmp eax, winapi_returnaddr
		jne normal_case
		add esp, 4
		push[esp + 4]
		call function_hook
		add esp, 4
		retn
		normal_case :
		jmp[jump_back]
	}
}


bool create_exception_hook(DWORD hook_address)
{
	function_hook = hook_address;

	//Create Hook
	if (MH_CreateHook(GetProcAddress(GetModuleHandleA("KERNELBASE.dll"), "RaiseException"), &createexceptiona_hook, reinterpret_cast<LPVOID*>(&aRaiseException)) != MH_OK)
	{
		MessageBoxA(NULL, "ERROR: Failed to init trustcheck bypass!", "Ashma", MB_OK);
		exit(0);
	}

	if (MH_EnableHook(GetProcAddress(GetModuleHandleA("KERNELBASE.dll"), "RaiseException")) != MH_OK)
	{
		MessageBoxA(NULL, "ERROR: Failed to init trustcheck bypass!", "Ashma", MB_OK);
		exit(0);
	}

	jump_back = (DWORD)aRaiseException;
}
