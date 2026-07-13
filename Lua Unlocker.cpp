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
	logStream << "=== WoW Lua Unlocker - CastSpellByName Search ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;
	logStream.close();

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream.open(LogFile, std::ios::app);
	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// Search for CastSpellByName string
	BYTE searchBytes[] = { 'C', 'a', 's', 't', 'S', 'p', 'e', 'l', 'l', 'B', 'y', 'N', 'a', 'm', 'e', 0 };
	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	DWORD maxAddr = baseAddr + modInfo.SizeOfImage - 16;

	logStream << "Searching for 'CastSpellByName' string..." << std::endl;

	bool foundString = false;
	for (DWORD currentAddr = baseAddr; currentAddr < maxAddr; currentAddr++)
	{
		if (memcmp((void*)currentAddr, searchBytes, 16) == 0)
		{
			foundString = true;
			DWORD offset = currentAddr - baseAddr;
			logStream << "FOUND at address: 0x" << std::hex << currentAddr << std::endl;
			logStream << "Offset from base: 0x" << std::hex << offset << std::endl << std::endl;

			// Dump hex context
			logStream << "Hex dump (512 bytes before and after):" << std::endl;
			int startOffset = (offset >= 512) ? offset - 512 : 0;
			int endOffset = (offset + 16 + 512 < modInfo.SizeOfImage) ? offset + 16 + 512 : modInfo.SizeOfImage;

			for (int i = startOffset; i < endOffset; i++)
			{
				BYTE b = *(BYTE*)(baseAddr + i);
				if (i == offset) logStream << "[";
				logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
				if (i == offset + 15) logStream << "]";
				if ((i - startOffset + 1) % 16 == 0) logStream << std::endl;
			}
			logStream << std::dec << std::endl << std::endl;
			break;
		}
	}

	if (!foundString)
	{
		logStream << "ERROR: 'CastSpellByName' string not found in module!" << std::endl;
	}

	logStream.close();

	MessageBox(nullptr, L"Search complete. Check C:\\WoW_Lua_Unlocker_Log.txt", L"Info", MB_OK);

	return 1;
}
