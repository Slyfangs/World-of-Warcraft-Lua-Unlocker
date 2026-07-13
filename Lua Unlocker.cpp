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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Patching CastSpellByName ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// The function table is at 0xaccdf0
	// CastSpellByID entry is at offset 0x00 (string pointer 0xa0ac08)
	// CastSpellByName entry is at offset 0x10 (string pointer 0xa0ac18)
	// Each entry is 8 bytes: [4 bytes string ptr][4 bytes function ptr]

	DWORD funcTableAddr = 0xaccdf0;
	DWORD castSpellByNameEntryAddr = funcTableAddr + 0x10;  // CastSpellByName entry starts here
	DWORD castSpellByNameStringAddr = *(DWORD*)castSpellByNameEntryAddr;
	DWORD castSpellByNameFuncAddr = *(DWORD*)(castSpellByNameEntryAddr + 4);

	logStream << "CastSpellByName Entry Location: 0x" << std::hex << castSpellByNameEntryAddr << std::endl;
	logStream << "  String pointer (should be 0xa0ac18): 0x" << castSpellByNameStringAddr << std::endl;
	logStream << "  Function pointer: 0x" << castSpellByNameFuncAddr << std::endl << std::endl;

	if (castSpellByNameStringAddr != 0xa0ac18)
	{
		logStream << "ERROR: String pointer doesn't match expected value!" << std::endl;
		logStream << "Expected: 0xa0ac18, Got: 0x" << std::hex << castSpellByNameStringAddr << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: CastSpellByName entry not found at expected location!", L"Error", MB_OK);
		return 1;
	}

	logStream << "Found CastSpellByName entry at expected location!" << std::endl;
	logStream << "Function pointer address: 0x" << std::hex << (castSpellByNameEntryAddr + 4) << std::endl << std::endl;

	// Now patch the function pointer to a simple return function
	// We'll replace it with a pointer to a NOP function or patch the actual function
	// For now, let's look at what function is there and patch it

	logStream << "Patching strategy: Modifying function to skip security check..." << std::endl;
	logStream << "Original function at: 0x" << std::hex << castSpellByNameFuncAddr << std::endl << std::endl;

	// Dump the function bytes to see what we're dealing with
	logStream << "First 64 bytes of CastSpellByName function:" << std::endl;
	for (int i = 0; i < 64; i++)
	{
		BYTE b = *(BYTE*)(castSpellByNameFuncAddr + i);
		logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
		if ((i + 1) % 16 == 0) logStream << std::endl;
	}
	logStream << std::dec << std::endl << std::endl;

	// Enable write access to the memory location
	DWORD oldProtect = 0;
	if (!VirtualProtect((void*)(castSpellByNameFuncAddr), 64, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		logStream << "ERROR: Failed to change memory protection!" << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: Could not modify memory protection!", L"Error", MB_OK);
		return 1;
	}

	logStream << "Memory protection changed successfully (old: 0x" << std::hex << oldProtect << ")" << std::endl << std::endl;

	// Patch: Replace the security check with a NOP sled and early return
	// x86: C3 = RET, 90 = NOP, 55 8B EC = PUSH EBP; MOV EBP, ESP
	
	BYTE* patchAddr = (BYTE*)castSpellByNameFuncAddr;
	
	logStream << "Applying patch..." << std::endl;
	logStream << "Replacing function with NOP sled + RET (C3)" << std::endl;

	// Patch: Write RET instruction at the start to bypass the entire function
	BYTE patchCode[] = { 
		0xC3,                          // RET - immediately return, bypassing security check
		0x90, 0x90, 0x90, 0x90,        // NOP sled for safety
		0x90, 0x90, 0x90, 0x90
	};

	memcpy(patchAddr, patchCode, sizeof(patchCode));

	logStream << "Patch applied!" << std::endl;
	logStream << "New function bytes:" << std::endl;
	for (int i = 0; i < 16; i++)
	{
		BYTE b = *(BYTE*)(castSpellByNameFuncAddr + i);
		logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
	}
	logStream << std::dec << std::endl << std::endl;

	// Restore original protection
	DWORD newProtect = 0;
	if (!VirtualProtect((void*)(castSpellByNameFuncAddr), 64, oldProtect, &newProtect))
	{
		logStream << "WARNING: Failed to restore memory protection!" << std::endl;
	}

	logStream << "Memory protection restored." << std::endl << std::endl;
	logStream << "=== PATCHING COMPLETE ===" << std::endl;
	logStream << "CastSpellByName is now unlocked!" << std::endl;

	logStream.close();

	MessageBox(nullptr, L"CastSpellByName patched successfully!\r\nCheck C:\\WoW_Lua_Unlocker_Log.txt for details", L"Success", MB_OK);

	return 1;
}
