#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// Lua state and related functions
typedef void* lua_State;

// Original CastSpellByName function at 0x540310
typedef int(__stdcall *pCastSpellByName)(const char* spellName);

// Function to call CastSpellByID which we know works
typedef int(__stdcall *pCastSpellByID)(int spellID);

pCastSpellByName OriginalCastSpellByName = (pCastSpellByName)0x540310;
pCastSpellByID CastSpellByID = (pCastSpellByID)0x53e060;

// Hook function that properly calls the original
__declspec(naked) int CastSpellByName_Hook()
{
	__asm
	{
		push ebp
		mov ebp, esp
		
		// EBP+8 = first parameter (spell name string from Lua)
		mov eax, [ebp + 8]
		
		// Just call the original function directly with the spell name
		// But we need to make sure the stack is set up correctly
		push eax
		call dword ptr [OriginalCastSpellByName]
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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Calling Original with Proper Setup ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	DWORD funcAddr = 0x540310;

	logStream << "Original CastSpellByName: 0x" << std::hex << funcAddr << std::endl;
	logStream << "Working CastSpellByID: 0x53e060" << std::endl << std::endl;

	logStream << "The original function is trying to look up the spell by name." << std::endl;
	logStream << "The problem: it's likely calling a lookup function that needs initialization." << std::endl << std::endl;

	logStream << "New strategy: Find what function is being called at offset 0x1D" << std::endl;
	logStream << "That CALL at 0x54031D is the spell name lookup." << std::endl;
	logStream << "Let's hook that instead!" << std::endl << std::endl;

	// The CALL at 0x54031D calls a lookup function
	// Let's find what it is by looking at the instruction
	BYTE* pCall = (BYTE*)(funcAddr + 0x1D);
	logStream << "Instruction at 0x54031D: ";
	for (int i = 0; i < 5; i++)
	{
		logStream << std::hex << std::setfill('0') << std::setw(2) << (int)pCall[i] << " ";
	}
	logStream << std::endl;

	// E8 = CALL with relative offset
	if (pCall[0] == 0xE8)
	{
		int relOffset = *(int*)(pCall + 1);
		DWORD callTarget = (DWORD)(pCall + 5) + relOffset;  // EIP after CALL + offset
		logStream << "CALL relative offset: 0x" << std::hex << (relOffset & 0xFFFFFFFF) << std::endl;
		logStream << "CALL target: 0x" << callTarget << std::endl << std::endl;

		logStream << "This is the spell name lookup function!" << std::endl;
		logStream << "It takes: ESI = spell name string, EAX = some parameter (value 1)" << std::endl;
		logStream << "It returns: EAX = spell ID (or 0 if not found)" << std::endl << std::endl;
	}

	logStream << "Strategy: We need to patch the CastSpellByName function to bypass the lookup" << std::endl;
	logStream << "and instead use CastSpellByID directly with a test ID." << std::endl << std::endl;

	// Enable write access
	DWORD oldProtect = 0;
	VirtualProtect((void*)funcAddr, 512, PAGE_EXECUTE_READWRITE, &oldProtect);

	logStream << "Attempting to patch CastSpellByName to return 1 (success)..." << std::endl;
	
	// Replace entire function with a simple return 1
	BYTE* pFunc = (BYTE*)funcAddr;
	pFunc[0] = 0xB8;  // MOV EAX, 1
	pFunc[1] = 0x01;
	pFunc[2] = 0x00;
	pFunc[3] = 0x00;
	pFunc[4] = 0x00;
	pFunc[5] = 0xC3;  // RET

	VirtualProtect((void*)funcAddr, 512, oldProtect, &newProtect);

	logStream << "Patched. Now the function just returns 1." << std::endl;
	logStream << "But this won't cast spells - it just prevents errors." << std::endl << std::endl;

	logStream << "The real issue: We need game context (player object, etc) to cast spells." << std::endl;
	logStream << "The original function is setting up that context." << std::endl;
	logStream << "Skipping it causes crashes." << std::endl << std::endl;

	logStream << "Solution: We need to find how CastSpellByID is called from chat commands." << std::endl;
	logStream << "That code already has the proper context set up." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Analysis complete. Trying different approach next.", L"Info", MB_OK);

	return 1;
}
