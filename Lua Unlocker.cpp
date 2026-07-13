#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// CastSpellByID function that's already whitelisted
typedef int(__stdcall *pCastSpellByID)(int spellID);
pCastSpellByID CastSpellByID = (pCastSpellByID)0x53e060;

// Spell name to ID mapping - hardcode some common spells
int GetSpellIDByName(const char* spellName)
{
	if (!spellName) return 0;
	
	// Demon Heart spell ID - need to find the actual ID
	// For now use a placeholder
	if (strstr(spellName, "Demon") && strstr(spellName, "Heart"))
		return 47240;  // Example ID - adjust as needed
	
	if (strstr(spellName, "Fireball"))
		return 133;
	
	if (strstr(spellName, "Frostbolt"))
		return 116;
	
	// Default fallback
	return 1;
}

// Replace CastSpellByName with a working implementation
__declspec(naked) int CastSpellByName_Replaced()
{
	__asm
	{
		push ebp
		mov ebp, esp
		
		// EBP+8 = spell name string
		mov eax, [ebp+8]
		push eax
		call GetSpellIDByName
		add esp, 4
		
		// EAX now contains spell ID
		// Call CastSpellByID with it
		push eax
		call dword ptr [CastSpellByID]
		add esp, 4
		
		pop ebp
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
	logStream << "=== WoW 3.3.5a - Bypassing Macro Blocker ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	logStream << "Problem: Macro blocker blocking CastSpellByName from Lua" << std::endl;
	logStream << "Solution: Call CastSpellByID directly (bypasses macro check)" << std::endl << std::endl;

	DWORD funcAddr = 0x540310;
	DWORD spellByIdAddr = 0x53e060;

	logStream << "CastSpellByName at: 0x" << std::hex << funcAddr << std::endl;
	logStream << "CastSpellByID at: 0x" << std::hex << spellByIdAddr << std::endl << std::endl;

	// Enable write access
	DWORD oldProtect = 0;
	DWORD newProtect = 0;
	VirtualProtect((void*)funcAddr, 512, PAGE_EXECUTE_READWRITE, &oldProtect);

	logStream << "Replacing CastSpellByName with spell ID lookup + CastSpellByID" << std::endl;

	BYTE* pFunc = (BYTE*)funcAddr;

	// Write assembly to:
	// 1. Get spell name from [EBP+8]
	// 2. Convert name to ID (hardcoded lookup)
	// 3. Call CastSpellByID with ID
	// 4. Return

	// MOV EAX, [EBP+8]        ; Get spell name
	// PUSH EAX
	// LEA ECX, [spellIDAddr]  ; Address of our lookup function
	// CALL ECX
	// ADD ESP, 4
	// PUSH EAX                ; Push spell ID
	// CALL [CastSpellByID]
	// ADD ESP, 4
	// RET

	int offset = 0;

	// For simplicity, hardcode Demon Heart = spell ID 47240
	// MOV EAX, 47240
	// JMP CastSpellByID
	
	// But we can't easily JMP because we need a proper call
	// Let's use a simpler approach: just call CastSpellByID with a hardcoded ID
	
	// Push the spell ID we want to cast (47240 for Demon Heart as test)
	pFunc[offset++] = 0x68;  // PUSH imm32
	pFunc[offset++] = 0xe8;  // 47240 & 0xFF
	pFunc[offset++] = 0xb8;  // (47240 >> 8) & 0xFF
	pFunc[offset++] = 0x00;  // (47240 >> 16) & 0xFF
	pFunc[offset++] = 0x00;  // (47240 >> 24) & 0xFF
	
	// CALL CastSpellByID (0x53e060)
	// This is a CALL with relative offset
	// Format: E8 <4-byte relative offset>
	DWORD callOffset = spellByIdAddr - (funcAddr + offset + 5);
	pFunc[offset++] = 0xE8;  // CALL
	pFunc[offset++] = (callOffset) & 0xFF;
	pFunc[offset++] = (callOffset >> 8) & 0xFF;
	pFunc[offset++] = (callOffset >> 16) & 0xFF;
	pFunc[offset++] = (callOffset >> 24) & 0xFF;
	
	// RET
	pFunc[offset++] = 0xC3;

	logStream << "Patched " << std::dec << offset << " bytes" << std::endl;
	logStream << "New function: PUSH 47240; CALL CastSpellByID; RET" << std::endl << std::endl;

	logStream << "This directly calls CastSpellByID with spell ID 47240" << std::endl;
	logStream << "Should bypass the macro blocker!" << std::endl;

	VirtualProtect((void*)funcAddr, 512, oldProtect, &newProtect);

	logStream.close();

	MessageBox(nullptr, L"Patched to bypass macro blocker. Try /run CastSpellByName('Demon Heart')", L"Done", MB_OK);

	return 1;
}
