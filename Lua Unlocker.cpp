#include <Windows.h>
#include <Psasi.h>
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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Adding Safety & Diagnostics ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	DWORD funcAddr = 0x540310;

	logStream << "Target function: 0x" << std::hex << funcAddr << std::endl;
	logStream << "Reverting patches to analyze what the function really needs..." << std::endl << std::endl;

	// Enable write access
	DWORD oldProtect = 0;
	if (!VirtualProtect((void*)funcAddr, 512, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		logStream << "ERROR: Failed to change memory protection!" << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: Could not modify memory!", L"Error", MB_OK);
		return 1;
	}

	// REVERT the patches first - let's see what the original function does
	logStream << "Reverting patches to original state..." << std::endl;

	BYTE* pRev1 = (BYTE*)(funcAddr + 0x2A);
	pRev1[0] = 0x75;
	pRev1[1] = 0x15;
	logStream << "  Reverted patch 1" << std::endl;

	BYTE* pRev2 = (BYTE*)(funcAddr + 0x52);
	pRev2[0] = 0x84;
	pRev2[1] = 0xC0;
	pRev2[2] = 0x0F;
	pRev2[3] = 0x84;
	pRev2[4] = 0x17;
	pRev2[5] = 0x02;
	logStream << "  Reverted patch 2" << std::endl;

	BYTE* pRev3 = (BYTE*)(funcAddr + 0x74);
	pRev3[0] = 0x84;
	pRev3[1] = 0xC0;
	pRev3[2] = 0x0F;
	pRev3[3] = 0x84;
	pRev3[4] = 0x17;
	pRev3[5] = 0x02;
	logStream << "  Reverted patch 3" << std::endl << std::endl;

	// Now let's look more carefully at what the function does
	logStream << "Analyzing function flow..." << std::endl;
	logStream << "The function takes a spell name string as parameter (ECX register typically)" << std::endl;
	logStream << "Looking at the assembly:" << std::endl;
	logStream << "  0x540310: PUSH EBP; MOV EBP, ESP - Standard function prologue" << std::endl;
	logStream << "  0x540314: SUB ESP, 0x2D0 - Allocates 0x2D0 bytes of local variables" << std::endl;
	logStream << "  0x540317: PUSH ESI" << std::endl;
	logStream << "  0x540318: MOV ESI, [EBP+8] - Get first parameter (spell name string)" << std::endl;
	logStream << "  0x54031B: PUSH 1; PUSH ESI; CALL ... - Calls something with spell name" << std::endl << std::endl;

	logStream << "The first CALL at 0x54031D returns a result in EAX" << std::endl;
	logStream << "At 0x540322: TEST EAX, EAX; JNZ 0x540338" << std::endl;
	logStream << "  This is checking if the lookup succeeded!" << std::endl << std::endl;

	logStream << "If that fails, it tries an alternative at 0x540329:" << std::endl;
	logStream << "  PUSH 0xa0b110 (looks like a string or address)" << std::endl;
	logStream << "  PUSH ESI (spell name)" << std::endl;
	logStream << "  CALL something" << std::endl << std::endl;

	logStream << "The issue: The function is looking up the spell by name and failing." << std::endl;
	logStream << "It's not a security check - it's just that the spell lookup fails!" << std::endl << std::endl;

	logStream << "Better approach: Instead of patching the function," << std::endl;
	logStream << "We need to make sure the spell lookup succeeds." << std::endl;
	logStream << "This might require hooking the lookup function itself." << std::endl << std::endl;

	// Let's try a different approach: patch only the FIRST jump to allow fallback path
	logStream << "Trying new strategy: Allow both paths to execute (patch only the failing case)" << std::endl;
	logStream << "At 0x540322, the TEST EAX, EAX checks if lookup succeeded." << std::endl;
	logStream << "Instead of jumping to error, let's make it continue anyway." << std::endl << std::endl;

	// Patch: At 0x540322 (offset 0x12), change "85 C0 75 15" to "85 C0 90 90"
	// This makes it fall through regardless
	BYTE* pSafe = (BYTE*)(funcAddr + 0x12);
	logStream << "Patching at offset 0x12 (0x" << std::hex << (funcAddr + 0x12) << ")" << std::endl;
	logStream << "  Before: " << std::hex << (int)pSafe[0] << " " << (int)pSafe[1] << " " << (int)pSafe[2] << " " << (int)pSafe[3] << std::endl;
	pSafe[2] = 0x90;  // Replace JNZ with NOP
	pSafe[3] = 0x90;  // Replace offset with NOP
	logStream << "  After: " << std::hex << (int)pSafe[0] << " " << (int)pSafe[1] << " " << (int)pSafe[2] << " " << (int)pSafe[3] << std::endl << std::endl;

	// Restore protection
	DWORD newProtect = 0;
	VirtualProtect((void*)funcAddr, 512, oldProtect, &newProtect);

	logStream << "=== PATCHING COMPLETE ===" << std::endl;
	logStream << "Applied minimal safe patch to allow spell execution." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Diagnostics complete. Check log and try again.", L"Info", MB_OK);

	return 1;
}
