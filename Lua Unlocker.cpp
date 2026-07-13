#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// Simple hook: return 1 (success) immediately
__declspec(naked) int CastSpellByName_Hook()
{
	__asm
	{
		mov eax, 1          // EAX = 1 (success/true)
		ret                 // Return immediately
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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Direct Function Pointer Hook ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// The function table entry for CastSpellByName
	DWORD castSpellByNameTableEntry = 0xaccde8;
	DWORD castSpellByNameFuncPtr = castSpellByNameTableEntry + 4;  // +4 bytes for the function pointer

	logStream << "CastSpellByName table entry at: 0x" << std::hex << castSpellByNameTableEntry << std::endl;
	logStream << "Function pointer location at: 0x" << castSpellByNameFuncPtr << std::endl << std::endl;

	DWORD* pFuncPtr = (DWORD*)castSpellByNameFuncPtr;
	DWORD originalFunc = *pFuncPtr;

	logStream << "Original function pointer: 0x" << std::hex << originalFunc << std::endl;
	logStream << "Hook function address: 0x" << (DWORD)&CastSpellByName_Hook << std::endl << std::endl;

	// Change memory protection to allow writing
	DWORD oldProtect = 0;
	if (!VirtualProtect((void*)castSpellByNameFuncPtr, 4, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		logStream << "ERROR: Failed to change memory protection!" << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: Could not modify memory protection!", L"Error", MB_OK);
		return 1;
	}

	logStream << "Memory protection changed successfully" << std::endl;
	logStream << "Old protection: 0x" << std::hex << oldProtect << std::endl << std::endl;

	// Replace the function pointer
	logStream << "Replacing function pointer..." << std::endl;
	*pFuncPtr = (DWORD)&CastSpellByName_Hook;

	logStream << "New function pointer: 0x" << std::hex << *pFuncPtr << std::endl << std::endl;

	// Restore original protection
	DWORD newProtect = 0;
	VirtualProtect((void*)castSpellByNameFuncPtr, 4, oldProtect, &newProtect);

	logStream << "Memory protection restored" << std::endl << std::endl;

	// Verify the patch
	if (*pFuncPtr == (DWORD)&CastSpellByName_Hook)
	{
		logStream << "=== SUCCESS ===" << std::endl;
		logStream << "CastSpellByName function pointer has been successfully replaced!" << std::endl;
		logStream << "The Lua function should now work without restrictions." << std::endl;
	}
	else
	{
		logStream << "=== WARNING ===" << std::endl;
		logStream << "Pointer verification failed!" << std::endl;
		logStream << "Expected: 0x" << std::hex << (DWORD)&CastSpellByName_Hook << std::endl;
		logStream << "Got: 0x" << *pFuncPtr << std::endl;
	}

	logStream.close();

	MessageBox(nullptr, L"CastSpellByName hooked!\r\nCheck log and try: /run CastSpellByName(\"Fireball\")", L"Success", MB_OK);

	return 1;
}
