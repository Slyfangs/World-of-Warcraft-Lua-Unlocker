#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

int __stdcall DllMain(void* Module, unsigned long Reason, void*)
{
	if (Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	const char* LogFile = "C:\\WoW_Lua_Unlocker_Log.txt";

	std::ofstream logStream(LogFile);
	logStream << "=== WoW 3.3.5a Lua Unlocker - Finding Correct CastSpellByName Entry ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Searching for CastSpellByName string in function table..." << std::endl << std::endl;

	DWORD funcTableAddr = 0xaccdf0;
	DWORD castSpellByNameStringAddr = 0xa0ac18;

	// Search through the function table to find the CastSpellByName entry
	logStream << "Scanning function table from 0x" << std::hex << funcTableAddr << std::endl;
	logStream << "Looking for string pointer: 0x" << castSpellByNameStringAddr << std::endl << std::endl;

	int entryCount = 0;
	bool found = false;

	for (DWORD offset = 0; offset < 0x2000; offset += 8)  // Each entry is 8 bytes
	{
		DWORD* pEntry = (DWORD*)(funcTableAddr + offset);
		DWORD stringPtr = *pEntry;
		DWORD funcPtr = *(pEntry + 1);

		if (stringPtr == castSpellByNameStringAddr)
		{
			found = true;
			logStream << "FOUND CastSpellByName at offset 0x" << std::hex << offset << std::endl;
			logStream << "  Absolute address: 0x" << (funcTableAddr + offset) << std::endl;
			logStream << "  String pointer: 0x" << stringPtr << std::endl;
			logStream << "  Function pointer: 0x" << funcPtr << std::endl;
			logStream << "  Function pointer address (to patch): 0x" << (funcTableAddr + offset + 4) << std::endl << std::endl;
			break;
		}

		// Also log entries around "CastSpell" strings for reference
		if (stringPtr >= 0xa0ac00 && stringPtr <= 0xa0ad00)
		{
			logStream << "Entry #" << std::dec << entryCount << " at offset 0x" << std::hex << offset << ": ";
			logStream << "String=0x" << stringPtr << " Func=0x" << funcPtr << std::endl;
		}

		entryCount++;
		if (entryCount > 0x1000) break;  // Safety limit
	}

	if (!found)
	{
		logStream << "ERROR: Could not find CastSpellByName entry in function table!" << std::endl;
	}

	logStream.close();

	MessageBox(nullptr, found ? L"CastSpellByName entry found! Check log for address." : L"ERROR: CastSpellByName not found in table!", L"Info", MB_OK);

	return 1;
}
