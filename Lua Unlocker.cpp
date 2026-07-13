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
	logStream << "=== WoW 3.3.5a - Safe Implementation ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	logStream << "Previous crash: Calling function pointers incorrectly" << std::endl;
	logStream << "New approach: Use proper assembly call convention" << std::endl << std::endl;

	DWORD funcAddr = 0x540310;
	DWORD spellByIdAddr = 0x53e060;

	logStream << "CastSpellByName at: 0x" << std::hex << funcAddr << std::endl;
	logStream << "CastSpellByID at: 0x" << std::hex << spellByIdAddr << std::endl << std::endl;

	// Enable write access
	DWORD oldProtect = 0;
	DWORD newProtect = 0;
	VirtualProtect((void*)funcAddr, 512, PAGE_EXECUTE_READWRITE, &oldProtect);

	logStream << "Replacing CastSpellByName with safe assembly stub" << std::endl;

	BYTE* pFunc = (BYTE*)funcAddr;

	// Write safe assembly code that:
	// 1. Preserves stack frame
	// 2. Calls CastSpellByID with spell ID as direct CALL
	// 3. Returns properly

	int offset = 0;

	// PUSH EBP
	pFunc[offset++] = 0x55;
	
	// MOV EBP, ESP
	pFunc[offset++] = 0x8B;
	pFunc[offset++] = 0xEC;

	// PUSH 1                ; Spell ID for Fireball (safe test spell)
	pFunc[offset++] = 0x68;
	pFunc[offset++] = 0x85;  // 133 = Fireball
	pFunc[offset++] = 0x00;
	pFunc[offset++] = 0x00;
	pFunc[offset++] = 0x00;

	// CALL CastSpellByID (0x53e060)
	// Relative offset calculation: target - (current address + 5)
	DWORD callAddr = funcAddr + offset;
	int relOffset = spellByIdAddr - (callAddr + 5);
	
	pFunc[offset++] = 0xE8;  // CALL
	pFunc[offset++] = (relOffset) & 0xFF;
	pFunc[offset++] = (relOffset >> 8) & 0xFF;
	pFunc[offset++] = (relOffset >> 16) & 0xFF;
	pFunc[offset++] = (relOffset >> 24) & 0xFF;

	// POP EBP
	pFunc[offset++] = 0x5D;

	// RET
	pFunc[offset++] = 0xC3;

	logStream << "Patched " << std::dec << offset << " bytes" << std::endl;
	logStream << "Assembly:" << std::endl;
	logStream << "  PUSH EBP" << std::endl;
	logStream << "  MOV EBP, ESP" << std::endl;
	logStream << "  PUSH 133        ; Fireball spell ID" << std::endl;
	logStream << "  CALL 0x53e060   ; CastSpellByID" << std::endl;
	logStream << "  POP EBP" << std::endl;
	logStream << "  RET" << std::endl << std::endl;

	logStream << "Call offset: 0x" << std::hex << (relOffset & 0xFFFFFFFF) << std::endl;
	logStream << "This should safely call CastSpellByID with spell ID 133" << std::endl;

	VirtualProtect((void*)funcAddr, 512, oldProtect, &newProtect);

	logStream.close();

	MessageBox(nullptr, L"Safe patch applied. Try /run CastSpellByName('Fireball')", L"Done", MB_OK);

	return 1;
}
