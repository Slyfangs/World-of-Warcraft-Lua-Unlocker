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
	logStream << "=== WoW 3.3.5a - Detailed Function Disassembly ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	DWORD funcAddr = 0x540310;
	BYTE* pFunc = (BYTE*)funcAddr;

	logStream << "Full disassembly of CastSpellByName at 0x" << std::hex << funcAddr << ":" << std::endl << std::endl;

	logStream << "0x540310: 55                    PUSH EBP" << std::endl;
	logStream << "0x540311: 8b ec                MOV EBP, ESP" << std::endl;
	logStream << "0x540313: 81 ec d0 02 00 00   SUB ESP, 0x2D0" << std::endl;
	logStream << "0x540319: 56                    PUSH ESI" << std::endl;
	logStream << "0x54031A: 8b 75 08             MOV ESI, [EBP+8]  ; ESI = spell name parameter" << std::endl;
	logStream << "0x54031D: 6a 01                PUSH 1             ; Parameter 1 = 1" << std::endl;
	logStream << "0x54031F: 56                    PUSH ESI            ; Parameter 0 = spell name" << std::endl;

	// Now let's look at what comes after
	logStream << std::endl << "Bytes after 0x54031F:" << std::endl;
	for (int i = 0x20; i < 0x50; i++)
	{
		logStream << std::hex << std::setfill('0') << std::setw(2) << (int)pFunc[i] << " ";
		if ((i - 0x20 + 1) % 16 == 0) logStream << std::endl;
	}
	logStream << std::endl << std::endl;

	logStream << "Looking for the CALL instruction (0xE8)..." << std::endl;
	for (int i = 0x20; i < 0x100; i++)
	{
		if (pFunc[i] == 0xE8)
		{
			logStream << "Found CALL at offset 0x" << std::hex << i << " (address 0x" << (funcAddr + i) << ")" << std::endl;
			
			int relOffset = *(int*)(pFunc + i + 1);
			DWORD callTarget = funcAddr + i + 5 + relOffset;
			
			logStream << "  Relative offset: 0x" << std::hex << (relOffset & 0xFFFFFFFF) << std::endl;
			logStream << "  Call target: 0x" << callTarget << std::endl;
			logStream << "  (This is the spell lookup function)" << std::endl << std::endl;
		}
	}

	logStream << std::endl << "Strategy: Hook the spell lookup function to return a valid spell ID." << std::endl;
	logStream << "Or: Replace the entire CastSpellByName with a working implementation." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Dumped full disassembly. Check log.", L"Info", MB_OK);

	return 1;
}
