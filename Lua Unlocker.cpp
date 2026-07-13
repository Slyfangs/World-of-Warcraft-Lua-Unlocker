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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Patching Security Checks ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	DWORD funcAddr = 0x540310;

	logStream << "Target function: 0x" << std::hex << funcAddr << std::endl;
	logStream << "Patching security check jumps..." << std::endl << std::endl;

	// Enable write access
	DWORD oldProtect = 0;
	if (!VirtualProtect((void*)funcAddr, 512, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		logStream << "ERROR: Failed to change memory protection!" << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: Could not modify memory!", L"Error", MB_OK);
		return 1;
	}

	logStream << "Memory protection changed to READWRITE" << std::endl << std::endl;

	// Patch 1: At offset 0x2A (0x54032A) - JNE instruction that blocks the spell
	// Original: 75 15 (JNE 0x15)
	// Patch: 90 90 (NOP NOP) - just continue to the spell casting code
	BYTE* pPatch1 = (BYTE*)(funcAddr + 0x2A);
	logStream << "Patch #1 at offset 0x2A (0x" << std::hex << (funcAddr + 0x2A) << ")" << std::endl;
	logStream << "  Before: " << std::hex << (int)pPatch1[0] << " " << (int)pPatch1[1] << std::endl;
	pPatch1[0] = 0x90;  // NOP
	pPatch1[1] = 0x90;  // NOP
	logStream << "  After: " << std::hex << (int)pPatch1[0] << " " << (int)pPatch1[1] << std::endl << std::endl;

	// Patch 2: At offset 0x52 (0x540362) - Another TEST and long JE
	// Original: 84 C0 0F 84 (TEST EAX, EAX; JE long offset)
	// We need to skip this jump. Replace with NOPs
	BYTE* pPatch2 = (BYTE*)(funcAddr + 0x52);
	logStream << "Patch #2 at offset 0x52 (0x" << std::hex << (funcAddr + 0x52) << ")" << std::endl;
	logStream << "  Before: ";
	for (int i = 0; i < 6; i++) logStream << std::hex << (int)pPatch2[i] << " ";
	logStream << std::endl;
	// Replace "84 C0 0F 84 XX XX XX XX" with "90 90 90 90 90 90"
	for (int i = 0; i < 6; i++) pPatch2[i] = 0x90;
	logStream << "  After: ";
	for (int i = 0; i < 6; i++) logStream << std::hex << (int)pPatch2[i] << " ";
	logStream << std::endl << std::endl;

	// Patch 3: At offset 0x74 (0x540384) - Another TEST and long JE
	BYTE* pPatch3 = (BYTE*)(funcAddr + 0x74);
	logStream << "Patch #3 at offset 0x74 (0x" << std::hex << (funcAddr + 0x74) << ")" << std::endl;
	logStream << "  Before: ";
	for (int i = 0; i < 6; i++) logStream << std::hex << (int)pPatch3[i] << " ";
	logStream << std::endl;
	// Replace "84 C0 0F 84 XX XX XX XX" with "90 90 90 90 90 90"
	for (int i = 0; i < 6; i++) pPatch3[i] = 0x90;
	logStream << "  After: ";
	for (int i = 0; i < 6; i++) logStream << std::hex << (int)pPatch3[i] << " ";
	logStream << std::endl << std::endl;

	// Restore protection
	DWORD newProtect = 0;
	VirtualProtect((void*)funcAddr, 512, oldProtect, &newProtect);

	logStream << "Memory protection restored" << std::endl << std::endl;
	logStream << "=== PATCHING COMPLETE ===" << std::endl;
	logStream << "All security checks have been bypassed!" << std::endl;
	logStream << "CastSpellByName should now work." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Security checks patched!\r\nTry: /run CastSpellByName(\"Demon Heart\")", L"Success", MB_OK);

	return 1;
}
