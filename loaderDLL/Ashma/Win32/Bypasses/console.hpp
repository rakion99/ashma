#pragma once
#include "../Rblx/DataModel.hpp"

namespace console
{
	namespace init
	{
		inline FORCEINLINE void console(const char* const title) // Bypass console made by Louka?
		{
			DWORD old;
			VirtualProtect(&FreeConsole, 1, PAGE_EXECUTE_READWRITE, &old);
			*reinterpret_cast<uint8_t*>(&FreeConsole) = 0xC3;
			VirtualProtect(&FreeConsole, 1, old, &old);
			AllocConsole();
			SetConsoleTitleA(title);
			FILE* t_dummy = new FILE{};
			freopen_s(&t_dummy, "CONOUT$", "w", stdout);
			freopen_s(&t_dummy, "CONOUT$", "w", stderr);
			freopen_s(&t_dummy, "CONIN$", "r", stdin);
		}
	}
}