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
	logStream << "=== WoW Lua Unlocker - Finding Code References to Data ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;
	logStream.close();

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream.open(LogFile, std::ios::app);
	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl;
	logStream << "Looking for references to data at: 0x" << std::hex << 0xaccde8 << std::endl << std::endl;

	// Search for references to 0xaccde8 (where CastSpellByName ref is stored)
	DWORD targetDataAddr = 0xaccde8;
	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	DWORD codeStart = baseAddr + 0x1000;
	DWORD codeEnd = baseAddr + modInfo.SizeOfImage;

	logStream << "Searching for code that references this data location..." << std::endl << std::endl;

	int refCount = 0;
	for (DWORD currentAddr = codeStart; currentAddr < codeEnd - 4; currentAddr++)
	{
		DWORD* pAddr = (DWORD*)currentAddr;
		if (*pAddr == targetDataAddr)
		{
			refCount++;
			DWORD offset = currentAddr - baseAddr;
			logStream << "Code Reference #" << refCount << " at: 0x" << std::hex << currentAddr << " (offset 0x" << offset << ")" << std::endl;

			// Dump 256 bytes of code context
			DWORD contextStart = (offset >= 128) ? offset - 128 : 0;
			DWORD contextEnd = (offset + 128 < modInfo.SizeOfImage) ? offset + 128 : modInfo.SizeOfImage;

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

			if (refCount >= 5)
			{
				logStream << "... (showing first 5 code references)" << std::endl;
				break;
			}
		}
	}

	if (refCount == 0)
	{
		logStream << "No code references found to data address." << std::endl;
		logStream << "Trying alternative search for nearby addresses..." << std::endl << std::endl;

		// Try searching for addresses within 0x1000 bytes of our target
		for (DWORD currentAddr = codeStart; currentAddr < codeEnd - 4; currentAddr++)
		{
			DWORD* pAddr = (DWORD*)currentAddr;
			DWORD diff = (*pAddr > targetDataAddr) ? *pAddr - targetDataAddr : targetDataAddr - *pAddr;
			if (diff < 0x1000 && diff > 0)
			{
				refCount++;
				DWORD offset = currentAddr - baseAddr;
				logStream << "Nearby Reference #" << refCount << " at: 0x" << std::hex << currentAddr << " points to: 0x" << *pAddr << std::endl;

				if (refCount >= 3) break;
			}
		}
	}

	logStream.close();

	MessageBox(nullptr, L"Code reference search complete. Check C:\\WoW_Lua_Unlocker_Log.txt", L"Info", MB_OK);

	return 1;
}
