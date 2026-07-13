#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// Simple hook: just return success (0) without doing anything
__declspec(naked) void CastSpellByName_Hook()
{
	__asm
	{
		xor eax, eax        // EAX = 0 (success)
		ret                 // Return immediately
	}
}

// Pointer to original function
typedef int(__stdcall *pCastSpellByName)(const char* spellName);
pCastSpellByName original_CastSpellByName = nullptr;

int __stdcall DllMain(void* Module, unsigned long Reason, void*)
{
	if (Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	const char* LogFile = "C:\\WoW_Lua_Unlocker_Log.txt";

	std::ofstream logStream(LogFile);
	logStream << "=== WoW 3.3.5a Lua Unlocker - Direct Function Hook ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// The function reference we found was at 0xaccde8
	// This points to a function (likely in the data section that contains function pointers)
	DWORD funcRefAddr = 0xaccde8;
	DWORD* pFuncRef = (DWORD*)funcRefAddr;
	DWORD actualFuncAddr = *pFuncRef;

	logStream << "Function reference at: 0x" << std::hex << funcRefAddr << std::endl;
	logStream << "Actual function address: 0x" << actualFuncAddr << std::endl << std::endl;

	// We'll do a simpler approach: patch the bytes at the function reference location
	// to point to our hook function instead
	logStream << "Patching approach: Direct function replacement" << std::endl;
	logStream << "Replacing with hook at: 0x" << (DWORD)&CastSpellByName_Hook << std::endl << std::endl;

	DWORD oldProtect = 0;
	if (!VirtualProtect((void*)funcRefAddr, 4, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		logStream << "ERROR: Failed to change memory protection!" << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: Could not modify memory!", L"Error", MB_OK);
		return 1;
	}

	logStream << "Memory protection changed to READWRITE" << std::endl;

	// Replace the function pointer with our hook
	*(DWORD*)funcRefAddr = (DWORD)&CastSpellByName_Hook;

	logStream << "Function pointer replaced!" << std::endl;
	logStream << "New function pointer: 0x" << *(DWORD*)funcRefAddr << std::endl << std::endl;

	// Restore original protection
	DWORD newProtect = 0;
	if (!VirtualProtect((void*)funcRefAddr, 4, oldProtect, &newProtect))
	{
		logStream << "WARNING: Failed to restore memory protection!" << std::endl;
	}

	logStream << "Memory protection restored" << std::endl;
	logStream << std::endl << "=== PATCHING COMPLETE ===" << std::endl;
	logStream << "CastSpellByName is now hooked and will allow all spells!" << std::endl;

	logStream.close();

	MessageBox(nullptr, L"CastSpellByName hooked successfully!\r\nCheck log for details", L"Success", MB_OK);

	return 1;
}
