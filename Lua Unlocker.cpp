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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Aggressive Patching ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	DWORD funcAddr = 0x540310;

	logStream << "Target function: 0x" << std::hex << funcAddr << std::endl;

	// Enable write access
	DWORD oldProtect = 0;
	if (!VirtualProtect((void*)funcAddr, 1024, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		logStream << "ERROR: Failed to change memory protection!" << std::endl;
		logStream.close();
		MessageBox(nullptr, L"ERROR: Could not modify memory!", L"Error", MB_OK);
		return 1;
	}

	logStream << "Memory protection changed" << std::endl << std::endl;

	// Strategy: Replace the entire function with a stub that does what we need
	// We'll make it call the working CastSpellByID function instead
	
	// But first, let's try a simpler approach: NOP out ALL the security/validation checks
	// Dump the function and find every TEST/CMP followed by conditional jump and NOP them
	
	logStream << "Patching all validation jumps..." << std::endl;

	BYTE* pFunc = (BYTE*)funcAddr;
	int patchCount = 0;

	// Scan through the function and NOP out all conditional jumps
	for (int i = 0; i < 512; i++)
	{
		BYTE b = pFunc[i];
		
		// Look for TEST or CMP followed by conditional jump
		// Pattern: 85 C0 (TEST EAX, EAX) followed by 0F 84/85/8F/8E (long jump) or 74/75/78/79 (short jump)
		if (b == 0x85 && pFunc[i + 1] == 0xC0)  // TEST EAX, EAX
		{
			if (pFunc[i + 2] == 0x0F && (pFunc[i + 3] >= 0x80 && pFunc[i + 3] <= 0x8F))  // Long conditional jump
			{
				logStream << "Patching TEST+longJump at offset 0x" << std::hex << i << std::endl;
				pFunc[i + 2] = 0x90;  // NOP the 0x0F
				pFunc[i + 3] = 0x90;  // NOP the jump opcode
				pFunc[i + 4] = 0x90;  // NOP the offset
				pFunc[i + 5] = 0x90;  // NOP the offset
				pFunc[i + 6] = 0x90;  // NOP the offset
				pFunc[i + 7] = 0x90;  // NOP the offset
				patchCount++;
			}
			else if ((pFunc[i + 2] >= 0x70 && pFunc[i + 2] <= 0x7F) || pFunc[i + 2] == 0xEB)  // Short jump
			{
				logStream << "Patching TEST+shortJump at offset 0x" << std::hex << i << std::endl;
				pFunc[i + 2] = 0x90;  // NOP the jump
				pFunc[i + 3] = 0x90;  // NOP the offset
				patchCount++;
			}
		}
		
		// Also look for 84 C0 (TEST AL, AL) which is sometimes used for character functions
		if (b == 0x84 && pFunc[i + 1] == 0xC0)
		{
			if (pFunc[i + 2] == 0x0F && (pFunc[i + 3] >= 0x80 && pFunc[i + 3] <= 0x8F))
			{
				logStream << "Patching TEST+longJump (AL version) at offset 0x" << std::hex << i << std::endl;
				pFunc[i + 2] = 0x90;
				pFunc[i + 3] = 0x90;
				pFunc[i + 4] = 0x90;
				pFunc[i + 5] = 0x90;
				pFunc[i + 6] = 0x90;
				pFunc[i + 7] = 0x90;
				patchCount++;
			}
		}
	}

	logStream << "Patched " << std::dec << patchCount << " validation checks" << std::endl << std::endl;

	// Now handle the specific jumps we know about
	logStream << "Applying targeted patches..." << std::endl;

	// Offset 0x2A: JNE 15
	BYTE* pJNE1 = (BYTE*)(funcAddr + 0x2A);
	if (pJNE1[0] == 0x75)
	{
		logStream << "Patching JNE at 0x2A" << std::endl;
		pJNE1[0] = 0x90;
		pJNE1[1] = 0x90;
	}

	// The issue is that spell lookup is failing. Let's try a different approach:
	// Make the function return 1 immediately (success) and hope that's enough
	logStream << std::endl << "Applying ultimate patch: Replace function with instant success stub" << std::endl;
	
	// Write a simple function that just returns 1:
	// C7 45 F0 01 00 00 00  MOV DWORD [EBP-0x10], 1
	// 8B 45 F0              MOV EAX, [EBP-0x10]
	// C9                    LEAVE
	// C3                    RET
	
	pFunc[0] = 0xB8;  // MOV EAX, imm32
	pFunc[1] = 0x01;
	pFunc[2] = 0x00;
	pFunc[3] = 0x00;
	pFunc[4] = 0x00;
	pFunc[5] = 0xC3;  // RET
	
	logStream << "Replaced function prologue with: MOV EAX, 1; RET" << std::endl;

	// Restore protection
	DWORD newProtect = 0;
	VirtualProtect((void*)funcAddr, 1024, oldProtect, &newProtect);

	logStream << std::endl << "=== AGGRESSIVE PATCHING COMPLETE ===" << std::endl;
	logStream << "Function completely overwritten to return success immediately." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Aggressive patches applied. Try: /run CastSpellByName(\"Demon Heart\")", L"Done", MB_OK);

	return 1;
}
