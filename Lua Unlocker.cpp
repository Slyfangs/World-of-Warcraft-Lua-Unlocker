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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Analyzing Original CastSpellByName Function ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// The original CastSpellByName function
	DWORD originalFuncAddr = 0x540310;

	logStream << "Original CastSpellByName function at: 0x" << std::hex << originalFuncAddr << std::endl;
	logStream << "Dumping first 256 bytes of function..." << std::endl << std::endl;

	for (int i = 0; i < 256; i += 16)
	{
		logStream << "0x" << std::hex << std::setfill('0') << std::setw(8) << (originalFuncAddr + i) << ": ";
		for (int j = 0; j < 16; j++)
		{
			BYTE b = *(BYTE*)(originalFuncAddr + i + j);
			logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
		}
		logStream << "| ";
		for (int j = 0; j < 16; j++)
		{
			BYTE b = *(BYTE*)(originalFuncAddr + i + j);
			logStream << (isprint(b) ? (char)b : '.');
		}
		logStream << std::endl;
	}

	logStream << std::endl << "=== Looking for strings referenced by this function ===" << std::endl;

	// Search for string references in the function
	for (int i = 0; i < 256; i += 4)
	{
		DWORD* pAddr = (DWORD*)(originalFuncAddr + i);
		DWORD addr = *pAddr;

		// Check if this looks like a string address (in data section around 0xa0xxxx)
		if (addr >= 0xa00000 && addr <= 0xb00000)
		{
			logStream << "Potential string reference at offset 0x" << std::hex << i << ": 0x" << addr << std::endl;
			
			// Try to read the string
			try
			{
				char* pStr = (char*)addr;
				if (pStr && strlen(pStr) < 100)
				{
					logStream << "  String: \"" << pStr << "\"" << std::endl;
				}
			}
			catch (...)
			{
				logStream << "  (Could not read string)" << std::endl;
			}
		}
	}

	logStream << std::endl << "=== Searching for common patterns ===" << std::endl;

	// Search for CMP instructions that might be security checks
	BYTE* pFunc = (BYTE*)originalFuncAddr;
	int cmpCount = 0;

	for (int i = 0; i < 256; i++)
	{
		// CMP EAX, 0x?? = 83 F8 ??
		// CMP with immediate = 81 xx ?? ?? ?? ??
		// TEST instruction = 85 xx, 84 xx
		
		if ((pFunc[i] == 0x83 && pFunc[i + 1] == 0xF8) ||  // CMP EAX, imm8
			(pFunc[i] == 0x81 && (pFunc[i + 1] & 0x38) == 0x38) ||  // CMP r32, imm32
			(pFunc[i] == 0x85) ||  // TEST r/m32, r32
			(pFunc[i] == 0x84))    // TEST r/m8, r8
		{
			cmpCount++;
			logStream << "Found comparison/test at offset 0x" << std::hex << i << ": ";
			for (int j = 0; j < 6 && i + j < 256; j++)
			{
				logStream << std::hex << std::setfill('0') << std::setw(2) << (int)pFunc[i + j] << " ";
			}
			logStream << std::endl;

			if (cmpCount >= 5) break;
		}
	}

	logStream << std::endl << "=== Looking for conditional jumps (JE, JNE, JZ, etc.) ===" << std::endl;

	int jumpCount = 0;
	for (int i = 0; i < 256; i++)
	{
		BYTE b = pFunc[i];
		
		// Short conditional jumps: 0x70-0x7F
		// Long conditional jumps: 0x0F 0x80-0x8F
		
		if ((b >= 0x70 && b <= 0x7F) || (b == 0xEB))  // Short jumps
		{
			jumpCount++;
			BYTE offset = pFunc[i + 1];
			logStream << "Found jump at offset 0x" << std::hex << i << " (opcode 0x" << (int)b << "), target offset: 0x" << (int)offset << std::endl;

			if (jumpCount >= 5) break;
		}
		else if (b == 0x0F && i + 1 < 256 && pFunc[i + 1] >= 0x80 && pFunc[i + 1] <= 0x8F)  // Long jumps
		{
			jumpCount++;
			DWORD offset = *(DWORD*)(pFunc + i + 2);
			logStream << "Found long jump at offset 0x" << std::hex << i << " (opcode 0x" << (int)pFunc[i + 1] << "), offset: 0x" << offset << std::endl;

			if (jumpCount >= 5) break;
		}
	}

	logStream.close();

	MessageBox(nullptr, L"Function analysis complete. Check log.", L"Info", MB_OK);

	return 1;
}
