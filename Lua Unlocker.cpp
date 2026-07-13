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
	logStream << "=== WoW Lua Unlocker - Searching for Lua Function Checks ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;
	logStream.close();

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream.open(LogFile, std::ios::app);
	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// Search for "CastSpellByID" string which is right before CastSpellByName
	BYTE castSpellByIDBytes[] = { 'C', 'a', 's', 't', 'S', 'p', 'e', 'l', 'l', 'B', 'y', 'I', 'D', 0 };
	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	DWORD maxAddr = baseAddr + modInfo.SizeOfImage - 14;

	logStream << "Searching for 'CastSpellByID' string..." << std::endl;

	for (DWORD currentAddr = baseAddr; currentAddr < maxAddr; currentAddr++)
	{
		if (memcmp((void*)currentAddr, castSpellByIDBytes, 14) == 0)
		{
			DWORD offset = currentAddr - baseAddr;
			logStream << "Found 'CastSpellByID' at: 0x" << std::hex << currentAddr << " (offset 0x" << offset << ")" << std::endl;

			// Now search for references to this address
			logStream << "Searching for code references to this string..." << std::endl;
			DWORD refCount = 0;

			for (DWORD searchAddr = baseAddr; searchAddr < baseAddr + modInfo.SizeOfImage - 4; searchAddr++)
			{
				DWORD* pAddr = (DWORD*)searchAddr;
				if (*pAddr == currentAddr)
				{
					refCount++;
					DWORD refOffset = searchAddr - baseAddr;
					logStream << "  Reference #" << refCount << " at: 0x" << std::hex << searchAddr << " (offset 0x" << refOffset << ")" << std::endl;

					// Dump function context - look backwards for function start
					DWORD funcStart = (refOffset >= 256) ? refOffset - 256 : 0;
					logStream << "    Code context:" << std::endl << "    ";

					for (DWORD i = funcStart; i < refOffset + 64 && i < modInfo.SizeOfImage; i++)
					{
						BYTE b = *(BYTE*)(baseAddr + i);
						if (i == refOffset) logStream << "[REF:";
						logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
						if (i == refOffset + 3) logStream << "]";
						if ((i - funcStart + 1) % 16 == 0) logStream << std::endl << "    ";
					}
					logStream << std::dec << std::endl << std::endl;

					if (refCount >= 3) break;
				}
			}

			if (refCount > 0)
			{
				logStream << "Found " << refCount << " references to CastSpellByID. CastSpellByName should be nearby in the function table." << std::endl;
			}

			break;
		}
	}

	logStream.close();

	MessageBox(nullptr, L"Lua function reference search complete. Check C:\\WoW_Lua_Unlocker_Log.txt", L"Info", MB_OK);

	return 1;
}
