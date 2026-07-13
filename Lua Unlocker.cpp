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
	logStream << "=== WoW Lua Unlocker - Finding CastSpellByName References ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;
	logStream.close();

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream.open(LogFile, std::ios::app);
	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl;
	logStream << "CastSpellByName string at: 0x" << std::hex << 0x400000 + 0x60ac18 << std::endl << std::endl;

	// Search for references to 0x60ac18 or 0xa0ac18
	DWORD targetStringAddr = 0xa0ac18;  // Absolute address of "CastSpellByName"
	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	DWORD codeStart = baseAddr + 0x1000;  // .text section typically starts here
	DWORD codeEnd = baseAddr + modInfo.SizeOfImage;

	logStream << "Searching for references to CastSpellByName address..." << std::endl;
	logStream << "Looking for: 0x" << std::hex << targetStringAddr << std::endl << std::endl;

	int refCount = 0;
	for (DWORD currentAddr = codeStart; currentAddr < codeEnd - 4; currentAddr++)
	{
		// Look for direct address references (4-byte little-endian)
		DWORD* pAddr = (DWORD*)currentAddr;
		if (*pAddr == targetStringAddr)
		{
			refCount++;
			DWORD offset = currentAddr - baseAddr;
			logStream << "Reference #" << refCount << " found at: 0x" << std::hex << currentAddr << " (offset 0x" << offset << ")" << std::endl;

			// Dump 256 bytes of code context around this reference
			DWORD contextStart = (offset >= 128) ? offset - 128 : 0;
			DWORD contextEnd = (offset + 256 < modInfo.SizeOfImage) ? offset + 256 : modInfo.SizeOfImage;

			logStream << "  Code context (128 before, 128 after):" << std::endl << "  ";
			for (DWORD i = contextStart; i < contextEnd; i++)
			{
				BYTE b = *(BYTE*)(baseAddr + i);
				if (i == offset) logStream << "[REF:";
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
		logStream << "No direct address references found. The string might be referenced differently." << std::endl;
	}

	logStream.close();

	MessageBox(nullptr, L"Reference search complete. Check C:\\WoW_Lua_Unlocker_Log.txt", L"Info", MB_OK);

	return 1;
}
