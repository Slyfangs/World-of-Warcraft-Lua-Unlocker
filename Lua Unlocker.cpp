#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// The actual spell casting function that works
// CastSpellByID is at 0x53e060 and it works
typedef int(__stdcall *pCastSpellByID)(int spellID);

// We need to find a way to convert spell name to spell ID
// Or better: hook the actual spell casting

// Simple working version: just write assembly that calls CastSpellByID with a hardcoded spell ID
__declspec(naked) int CastSpellByName_Hooked()
{
	__asm
	{
		// EBP+8 = spell name string
		push ebp
		mov ebp, esp
		
		// For now, hardcode Demon Heart spell ID (assuming it exists)
		// We need to find this ID first
		// Let's just return 1 for testing
		mov eax, 1
		
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
	logStream << "=== WoW 3.3.5a Lua Unlocker - Finding Spell IDs ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Module Size: 0x" << std::hex << modInfo.SizeOfImage << std::endl << std::endl;

	// Search for "Demon Heart" string
	logStream << "Searching for spell names in data section..." << std::endl;

	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	const char* targetSpell = "Demon Heart";
	int spellRefCount = 0;

	for (DWORD searchAddr = baseAddr; searchAddr < baseAddr + modInfo.SizeOfImage - strlen(targetSpell); searchAddr++)
	{
		char* pStr = (char*)searchAddr;
		if (strcmp(pStr, targetSpell) == 0)
		{
			DWORD offset = searchAddr - baseAddr;
			logStream << "Found '" << targetSpell << "' at 0x" << std::hex << searchAddr << " (offset 0x" << offset << ")" << std::endl;
			spellRefCount++;

			// Now search for references to this address
			logStream << "  Searching for references to this string..." << std::endl;

			for (DWORD refAddr = baseAddr; refAddr < baseAddr + modInfo.SizeOfImage - 4; refAddr++)
			{
				DWORD* pAddr = (DWORD*)refAddr;
				if (*pAddr == searchAddr)
				{
					logStream << "    Reference at 0x" << std::hex << refAddr << " (offset 0x" << (refAddr - baseAddr) << ")" << std::endl;
				}
			}

			if (spellRefCount >= 3) break;
		}
	}

	logStream << std::endl << "Found " << std::dec << spellRefCount << " instances of spell name" << std::endl << std::endl;

	// Now search for spell name to ID mapping
	logStream << "Looking for spell ID lookup structures..." << std::endl;
	logStream << "Searching for common spell data patterns..." << std::endl << std::endl;

	// In WoW, spell data is often stored in tables with name->ID mappings
	// Let's search for the pattern: (string pointer, spell ID)

	for (DWORD searchAddr = baseAddr + 0x900000; searchAddr < baseAddr + modInfo.SizeOfImage - 8; searchAddr += 4)
	{
		DWORD* pEntry = (DWORD*)searchAddr;
		DWORD strAddr = pEntry[0];
		DWORD spellID = pEntry[1];

		// Check if first DWORD looks like a string address and second looks like a reasonable ID
		if (strAddr >= baseAddr && strAddr < baseAddr + modInfo.SizeOfImage &&
			spellID > 0 && spellID < 100000)
		{
			char* pStr = (char*)strAddr;
			try
			{
				if (strlen(pStr) > 2 && strlen(pStr) < 50 && isalpha(pStr[0]))
				{
					// Check if this is a spell name
					bool isSpellName = true;
					for (int i = 0; i < strlen(pStr); i++)
					{
						if (!isalpha(pStr[i]) && !isspace(pStr[i]) && pStr[i] != '-' && pStr[i] != '\'')
						{
							isSpellName = false;
							break;
						}
					}

					if (isSpellName && strstr(pStr, "Demon") != NULL)
					{
						logStream << "Found potential spell entry at 0x" << std::hex << searchAddr << std::endl;
						logStream << "  Name: \"" << pStr << "\"" << std::endl;
						logStream << "  ID: " << std::dec << spellID << std::endl << std::endl;
					}
				}
			}
			catch (...)
			{
			}
		}
	}

	logStream << "Analysis complete. Check spell IDs above." << std::endl;

	logStream.close();

	MessageBox(nullptr, L"Spell search complete. Check log for spell IDs.", L"Info", MB_OK);

	return 1;
}
