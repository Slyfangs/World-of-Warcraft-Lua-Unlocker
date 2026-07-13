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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Finding All References to CastSpellByName ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	DWORD castSpellByNameStringAddr = 0xa0ac18;

	logStream << "Searching entire module for references to CastSpellByName string (0xa0ac18)..." << std::endl << std::endl;

	int refCount = 0;
	for (DWORD searchAddr = baseAddr; searchAddr < baseAddr + modInfo.SizeOfImage - 4; searchAddr++)
	{
		DWORD* pAddr = (DWORD*)searchAddr;
		if (*pAddr == castSpellByNameStringAddr)
		{
			refCount++;
			DWORD offset = searchAddr - baseAddr;
			
			logStream << "Reference #" << std::dec << refCount << " at 0x" << std::hex << searchAddr << " (offset 0x" << offset << ")" << std::endl;

			// Dump context to understand what's happening
			DWORD contextStart = (offset >= 32) ? offset - 32 : 0;
			DWORD contextEnd = (offset + 32 < modInfo.SizeOfImage) ? offset + 32 : modInfo.SizeOfImage;

			logStream << "  Context:" << std::endl << "  ";
			for (DWORD i = contextStart; i < contextEnd; i++)
			{
				BYTE b = *(BYTE*)(baseAddr + i);
				if (i == offset) logStream << "[";
				logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
				if (i == offset + 3) logStream << "]";
				if ((i - contextStart + 1) % 16 == 0) logStream << std::endl << "  ";
			}
			logStream << std::dec << std::endl << std::endl;

			if (refCount >= 10)
			{
				logStream << "... (showing first 10 references)" << std::endl;
				break;
			}
		}
	}

	if (refCount == 0)
	{
		logStream << "No references found!" << std::endl;
		logStream << "The CastSpellByName security might be enforced differently." << std::endl;
		logStream << std::endl << "Alternative: Searching for the pattern in your original code..." << std::endl;
		logStream << "The old pattern was: 56 8B F1 8B 0D ? ? ? ? 8B 11 FF 92 ? ? ? ? 84 C0 74 10" << std::endl;
		logStream << "Let's search for '84 C0 74 10' which is the actual security check bytes..." << std::endl << std::endl;

		// Search for the security check pattern: 84 C0 74 10
		BYTE securityPattern[] = { 0x84, 0xC0, 0x74, 0x10 };
		for (DWORD searchAddr = baseAddr; searchAddr < baseAddr + modInfo.SizeOfImage - 4; searchAddr++)
		{
			if (memcmp((void*)searchAddr, securityPattern, 4) == 0)
			{
				int patternCount = (searchAddr == baseAddr) ? 1 : (patternCount + 1);
				DWORD offset = searchAddr - baseAddr;
				
				logStream << "Found '84 C0 74 10' pattern at 0x" << std::hex << searchAddr << " (offset 0x" << offset << ")" << std::endl;

				// Dump context
				DWORD contextStart = (offset >= 64) ? offset - 64 : 0;
				DWORD contextEnd = (offset + 64 < modInfo.SizeOfImage) ? offset + 64 : modInfo.SizeOfImage;

				logStream << "  Context:" << std::endl << "  ";
				for (DWORD i = contextStart; i < contextEnd; i++)
				{
					BYTE b = *(BYTE*)(baseAddr + i);
					if (i == offset) logStream << "[";
					logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
					if (i == offset + 3) logStream << "]";
					if ((i - contextStart + 1) % 16 == 0) logStream << std::endl << "  ";
				}
				logStream << std::dec << std::endl << std::endl;

				if (patternCount >= 3) break;
			}
		}
	}

	logStream.close();

	MessageBox(nullptr, L"Search complete. Check C:\\WoW_Lua_Unlocker_Log.txt", L"Info", MB_OK);

	return 1;
}
