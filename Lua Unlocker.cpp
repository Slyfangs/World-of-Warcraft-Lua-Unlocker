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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Finding Lua Registration Function ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;

	// Search for Lua API functions - look for "lua_" strings which are part of Lua API registration
	logStream << "Searching for Lua API registration patterns..." << std::endl;
	logStream << "Looking for references to function setup near CastSpellByName..." << std::endl << std::endl;

	// Find references to CastSpellByID function (which should be working)
	DWORD castSpellByIDStringAddr = 0xa0ac08;
	
	logStream << "Looking for CastSpellByID function setup..." << std::endl;
	
	// Search for the function table location again
	for (DWORD searchAddr = baseAddr; searchAddr < baseAddr + modInfo.SizeOfImage - 4; searchAddr++)
	{
		DWORD* pAddr = (DWORD*)searchAddr;
		if (*pAddr == castSpellByIDStringAddr)
		{
			DWORD offset = searchAddr - baseAddr;
			logStream << "Found CastSpellByID reference at 0x" << std::hex << searchAddr << " (offset 0x" << offset << ")" << std::endl;

			// This should be in a data table. Look at the structure
			DWORD tableBase = searchAddr - 8;  // Assume it's in a table
			logStream << "Table likely starts at 0x" << std::hex << tableBase << std::endl;

			// Dump more of the table
			logStream << "Table structure (looking for CastSpellByName nearby):" << std::endl;
			for (int i = -16; i < 64; i += 8)
			{
				DWORD* entry = (DWORD*)(tableBase + i);
				if (entry >= (DWORD*)baseAddr && entry < (DWORD*)(baseAddr + modInfo.SizeOfImage))
				{
					logStream << "  +0x" << std::hex << std::setfill('0') << std::setw(2) << (i & 0xFF) << ": String=0x" << *entry << " Func=0x" << *(entry + 1) << std::endl;
				}
			}
			break;
		}
	}

	// Now search for the actual code that's checking/filtering CastSpellByName
	logStream << std::endl << "Searching for security check code..." << std::endl;
	logStream << "Looking for comparison against CastSpellByName string..." << std::endl << std::endl;

	// Search for code that might be doing: if (functionName == "CastSpellByName") then block
	// This would typically involve LEA or MOV of the string address followed by a comparison
	
	int codeRefCount = 0;
	for (DWORD searchAddr = baseAddr + 0x1000; searchAddr < baseAddr + modInfo.SizeOfImage - 8; searchAddr++)
	{
		// Look for code that loads the CastSpellByName string address
		BYTE* pCode = (BYTE*)searchAddr;
		
		// Pattern: common ways to reference the string
		// C7 45 ? 18 AC A0 00 = MOV DWORD PTR [EBP+offset], 0xa0ac18
		// B8 18 AC A0 00 = MOV EAX, 0xa0ac18
		// etc.
		
		if ((pCode[0] == 0xB8 && *(DWORD*)(pCode + 1) == 0xa0ac18) ||
			(pCode[0] == 0xC7 && *(DWORD*)(pCode + 2) == 0xa0ac18) ||
			(pCode[0] == 0x68 && *(DWORD*)(pCode + 1) == 0xa0ac18))
		{
			codeRefCount++;
			DWORD offset = searchAddr - baseAddr;
			logStream << "Code reference #" << std::dec << codeRefCount << " to CastSpellByName string at 0x" << std::hex << searchAddr << " (offset 0x" << offset << ")" << std::endl;

			// Dump context
			DWORD contextStart = (offset >= 64) ? offset - 64 : 0;
			DWORD contextEnd = (offset + 64 < modInfo.SizeOfImage) ? offset + 64 : modInfo.SizeOfImage;

			logStream << "  Context:" << std::endl << "  ";
			for (DWORD i = contextStart; i < contextEnd; i++)
			{
				BYTE b = *(BYTE*)(baseAddr + i);
				if (i == offset) logStream << "[";
				logStream << std::hex << std::setfill('0') << std::setw(2) << (int)b << " ";
				if (i == offset + 7) logStream << "]";
				if ((i - contextStart + 1) % 16 == 0) logStream << std::endl << "  ";
			}
			logStream << std::dec << std::endl << std::endl;

			if (codeRefCount >= 5) break;
		}
	}

	logStream << "Found " << std::dec << codeRefCount << " code references to CastSpellByName string." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Analysis complete. Check log for findings.", L"Info", MB_OK);

	return 1;
}
