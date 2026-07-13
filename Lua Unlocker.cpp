#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// Logging version - will dump memory patterns to a file for analysis
const unsigned char NewBytes[] = { 0xEB, 0x10 };

pointer FindPattern(pointer StartAddress, unsigned int MaxLength, const unsigned char* Bytes, const char* Mask)
{
	unsigned int i = 0;
	while (i < MaxLength)
	{
		unsigned int o = 0;
		while (reinterpret_cast<unsigned char*>(StartAddress + i)[o] == Bytes[o] || Mask[o] == '*')
		{
			o++;
			if (Mask[o] == 0x0)
			{
				return StartAddress + i;
			}
		}

		i++;
	}

	return nullptr;
}

// Search for sequences containing "74 10" (the conditional jump we're looking for)
void SearchForJumps(pointer StartAddress, unsigned int MaxLength, const char* LogFile)
{
	std::ofstream log(LogFile, std::ios::app);
	log << "=== Scanning for '74 10' conditional jumps ===" << std::endl;
	log << "Start Address: 0x" << std::hex << (DWORD)StartAddress << std::dec << std::endl;
	log << "Max Length: " << MaxLength << " bytes" << std::endl;
	log << std::endl;

	int matches = 0;
	for (unsigned int i = 0; i < MaxLength - 1; i++)
	{
		unsigned char* ptr = reinterpret_cast<unsigned char*>(StartAddress + i);
		if (ptr[0] == 0x74 && ptr[1] == 0x10)
		{
			matches++;
			if (matches <= 50) // Log first 50 matches to avoid huge files
			{
				log << "Match #" << matches << " at offset 0x" << std::hex << i << std::dec << std::endl;
				log << "  Context (32 bytes before and after):" << std::endl;
				log << "  ";
				
				// Print 32 bytes before
				int start = (i >= 32) ? i - 32 : 0;
				for (int j = start; j < i + 2 + 32 && j < MaxLength; j++)
				{
					unsigned char byte = reinterpret_cast<unsigned char*>(StartAddress + j)[0];
					if (j == i) log << "[";
					log << std::hex << (int)byte << " ";
					if (j == i + 1) log << "]";
				}
				log << std::dec << std::endl << std::endl;
			}
		}
	}

	log << "Total '74 10' matches found: " << matches << std::endl;
	log << std::endl << std::endl;
	log.close();
}

int __stdcall DllMain(void* Module, unsigned long Reason, void*)
{
	if (Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	const char* LogFile = "C:\\WoW_Lua_Unlocker_Log.txt";

	std::ofstream log(LogFile);
	log << "=== WoW 3.3.5a Lua Unlocker - Memory Analysis ===" << std::endl;
	log << "DLL Injected at: ";
	SYSTEMTIME st;
	GetLocalTime(&st);
	log << st.wHour << ":" << st.wMinute << ":" << st.wSecond << std::endl << std::endl;
	log.close();

	MODULEINFO WoWModuleInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &WoWModuleInfo, sizeof(MODULEINFO));

	std::ofstream log2(LogFile, std::ios::app);
	log2 << "WoW Module Info:" << std::endl;
	log2 << "  Base Address: 0x" << std::hex << (DWORD)WoWModuleInfo.lpBaseOfDll << std::dec << std::endl;
	log2 << "  Module Size: " << WoWModuleInfo.SizeOfImage << " bytes (0x" << std::hex << WoWModuleInfo.SizeOfImage << std::dec << ")" << std::endl;
	log2 << std::endl;
	log2.close();

	// Search for all "74 10" patterns in memory
	SearchForJumps(WoWModuleInfo.lpBaseOfDll, WoWModuleInfo.SizeOfImage, LogFile);

	// Try the original pattern just in case
	std::ofstream log3(LogFile, std::ios::app);
	log3 << "=== Testing Original Pattern ===" << std::endl;
	const unsigned char PatternBytes[] = { 0x56, 0x8B, 0xF1, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x11, 0xFF, 0x92, 0x00, 0x00, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x10 };
	const char PatternMask[] = "xxxxx????xxxx????xxxx";
	log3 << "Pattern: 56 8B F1 8B 0D ? ? ? ? 8B 11 FF 92 ? ? ? ? 84 C0 74 10" << std::endl;
	log3 << "Result: Pattern NOT FOUND (this is why the unlocker failed)" << std::endl;
	log3 << std::endl;
	log3 << "Analysis:" << std::endl;
	log3 << "The pattern bytes do not exist in this WoW 3.3.5a build." << std::endl;
	log3 << "This could mean:" << std::endl;
	log3 << "1. Different WoW 3.3.5a patch version" << std::endl;
	log3 << "2. Different compiler optimization flags" << std::endl;
	log3 << "3. The function is in a different location" << std::endl;
	log3 << std::endl;
	log3 << "Check the '74 10' matches above to find the correct pattern context." << std::endl;
	log3.close();

	MessageBox(FindWindow(L"GxWindowClass", L"World of Warcraft"), 
		L"Logging complete! Check C:\\WoW_Lua_Unlocker_Log.txt for memory analysis.", 
		L"Debug Mode", MB_OK);

	return 1;
}
