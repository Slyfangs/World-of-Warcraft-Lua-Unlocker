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
	logStream << "=== WoW Lua Unlocker - Analyzing Lua Function Table ===" << std::endl;
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	logStream << "Injected: " << sysTime.wHour << ":" << sysTime.wMinute << ":" << sysTime.wSecond << std::endl << std::endl;
	logStream.close();

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

	logStream.open(LogFile, std::ios::app);
	logStream << "Module Base: 0x" << std::hex << (DWORD)modInfo.lpBaseOfDll << std::endl;
	logStream << "Function table at: 0xaccdf0" << std::endl;
	logStream << "CastSpellByID at: 0xa0ac08" << std::endl;
	logStream << "CastSpellByName at: 0xa0ac18" << std::endl << std::endl;

	DWORD baseAddr = (DWORD)modInfo.lpBaseOfDll;
	DWORD funcTableAddr = 0xaccdf0;

	// Find where CastSpellByName string is in the function table
	logStream << "=== Searching for CastSpellByName entry in function table ===" << std::endl;
	
	DWORD castSpellByNameStringAddr = 0xa0ac18;
	
	// Search around the function table for the CastSpellByName string reference
	for (DWORD offset = 0; offset < 0x1000; offset += 4)
	{
		DWORD* pAddr = (DWORD*)(funcTableAddr + offset);
		if (*pAddr == castSpellByNameStringAddr)
		{
			logStream << "Found CastSpellByName string reference at table offset: 0x" << std::hex << offset << std::endl;
			logStream << "Absolute address: 0x" << (funcTableAddr + offset) << std::endl;
			logStream << "This is " << std::dec << (offset / 4) << " entries from table start (assuming 4-byte entries)" << std::endl << std::endl;

			// Dump the function table entry structure (typically: name pointer, function pointer, arg count, security flags)
			logStream << "Entry structure around this offset:" << std::endl;
			DWORD entryStart = (offset >= 32) ? offset - 32 : 0;
			DWORD entryEnd = offset + 64;

			for (DWORD i = entryStart; i < entryEnd; i += 4)
			{
				DWORD* value = (DWORD*)(funcTableAddr + i);
				logStream << "  +0x" << std::hex << std::setfill('0') << std::setw(4) << i << ": 0x" << std::setfill('0') << std::setw(8) << *value << std::dec;
				
				// Try to identify what these might be
				if (*value == castSpellByNameStringAddr)
					logStream << "  <-- STRING REFERENCE";
				else if (*value >= baseAddr && *value < baseAddr + modInfo.SizeOfImage)
					logStream << "  <-- CODE/DATA IN MODULE";
				
				logStream << std::endl;
			}
			break;
		}
	}

	// Also dump a large section of the function table to see the pattern
	logStream << std::endl << "=== Full Function Table Dump (first 2KB) ===" << std::endl;
	for (DWORD i = 0; i < 0x800; i += 16)
	{
		logStream << "0x" << std::hex << std::setfill('0') << std::setw(4) << i << ": ";
		for (int j = 0; j < 4; j++)
		{
			DWORD* pVal = (DWORD*)(funcTableAddr + i + j * 4);
			logStream << std::hex << std::setfill('0') << std::setw(8) << *pVal << " ";
		}
		logStream << std::endl;
	}

	logStream.close();

	MessageBox(nullptr, L"Function table analysis complete. Check C:\\WoW_Lua_Unlocker_Log.txt", L"Info", MB_OK);

	return 1;
}
