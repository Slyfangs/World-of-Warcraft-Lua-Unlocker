#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// Hook for the spell lookup function
// Original: 0x84f280
// Takes: ESI = spell name string, EAX = param (1)
// Returns: EAX = spell ID (or 0 if not found)

typedef int(__stdcall *pOriginalSpellLookup)(const char* spellName, int param);
pOriginalSpellLookup OriginalSpellLookup = (pOriginalSpellLookup)0x84f280;

// Our hook function
__declspec(naked) int SpellLookup_Hook()
{
	__asm
	{
		// ESI = spell name
		// EAX = param (1)
		
		// Just return a valid spell ID for now
		// This prevents the crash
		mov eax, 0x1  // Return 1 (some valid spell ID)
		ret
	}
}

int __stdcall DllMain(void* Module, unsigned long Reason, void*)
{
	if (Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	const char* LogFile = "C:\\WoW_Lua_Unlocker_Log.txt";

	std::ofstream logStream(LogFile);
	logStream << "=== WoW 3.3.5a - Hooking Spell Lookup ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	logStream << "Target: Spell lookup function at 0x84f280" << std::endl;
	logStream << "Strategy: Replace first CALL in CastSpellByName (at 0x540332)" << std::endl;
	logStream << "with a call to our hook that returns a valid spell ID" << std::endl << std::endl;

	DWORD patchAddr = 0x540332;

	// Enable write access
	DWORD oldProtect = 0;
	DWORD newProtect = 0;
	VirtualProtect((void*)patchAddr, 256, PAGE_EXECUTE_READWRITE, &oldProtect);

	logStream << "Patching CALL instruction at 0x" << std::hex << patchAddr << std::endl;

	// The CALL instruction is: E8 49 EF 30 00
	// E8 = CALL opcode
	// 49 EF 30 00 = relative offset (0x30ef49)
	//
	// We need to calculate a new relative offset to point to our hook
	// But we don't have a good place to put our hook code
	//
	// Instead: Replace the entire function body with something simpler

	BYTE* pFunc = (BYTE*)0x540310;

	logStream << "Better strategy: Replace CastSpellByName entirely" << std::endl;
	logStream << "New implementation: Look up spell ID and call the working CastSpellByID" << std::endl << std::endl;

	// We'll create a minimal implementation that:
	// 1. Takes the spell name from [EBP+8]
	// 2. Hardcodes a spell ID lookup (for testing)
	// 3. Calls CastSpellByID with that ID
	// 4. Returns

	// For now, let's just make it return success without crashing
	// MOV EAX, 1
	// RET

	pFunc[0] = 0xB8;  // MOV EAX, imm32
	pFunc[1] = 0x01;  // = 1
	pFunc[2] = 0x00;
	pFunc[3] = 0x00;
	pFunc[4] = 0x00;
	pFunc[5] = 0xC3;  // RET

	logStream << "Patched function to return 1 (success)" << std::endl;
	logStream << "This allows Lua calls to work without crashing" << std::endl << std::endl;

	logStream << "However, this still won't actually CAST spells." << std::endl;
	logStream << "To properly implement it, we'd need to:" << std::endl;
	logStream << "1. Patch in a spell name -> ID lookup (hardcoded for now)" << std::endl;
	logStream << "2. Call the real CastSpellByID function with that ID" << std::endl;
	logStream << "3. Ensure proper game context/player object is available" << std::endl << std::endl;

	logStream << "The working CastSpellByID is at: 0x53e060" << std::endl;
	logStream << "We need to understand how it's called from chat commands." << std::endl;

	VirtualProtect((void*)patchAddr, 256, oldProtect, &newProtect);

	logStream.close();

	MessageBox(nullptr, L"Spell lookup hook applied. Try /run CastSpellByName('Demon Heart')", L"Info", MB_OK);

	return 1;
}
