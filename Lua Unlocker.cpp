#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <string>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// Logging version - will find CastSpellByName reference and dump surrounding code
const unsigned char NewBytes[] = { 0xEB, 0x10 };

// Search for a specific byte pattern
pointer FindPatternSimple(void* StartAddressVoid, unsigned int MaxLength, const unsigned char* Bytes, unsigned int BytesLen)
{
	DWORD StartAddress = reinterpret_cast<DWORD>(StartAddressVoid);
	for (unsigned int i = 0; i < MaxLength - BytesLen; i++)
	{
		bool match = true;
		for (unsigned int j = 0; j < BytesLen; j++)
		{
			if (reinterpret_cast<unsigned char*>(StartAddress + i)[j] != Bytes[j])
			{
				match = false;
				break;
			}
		}
		if (match)
		{
			return pointer(StartAddress + i);
		}
	}
	return nullptr;
}

int __stdcall DllMain(void* Module, unsigned long Reason, void*)
{
	if (Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	const char* LogFile = "C:\\WoW_Lua_Unlocker_Log.txt";

	std::ofstream log(LogFile);
	log << "=== WoW 3.3.5a Lua Unlocker - Finding CastSpellByName Security Check ===" << std::endl;
	log << "DLL Injected at: ";
	SYSTEMTIME st;
	GetLocalTime(&st);
	log << st.wHour << ":" << st.wMinute << ":" << st.wSecond << std::endl << std::endl;
	log.close();

	MODULEINFO WoWModuleInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &WoWModuleInfo, sizeof(MODULEINFO));

	std::ofstream log2(LogFile, std::ios::app);
	log2 << "WoW Module Info:" << std::endl;
	log2 << "  Base Address: 0x" << std::hex << reinterpret_cast<DWORD>(WoWModuleInfo.lpBaseOfDll) << std::dec << std::endl;
	log2 << "  Module Size: " << WoWModuleInfo.SizeOfImage << " bytes" << std::endl;
	log2 << std::endl;

	// Search for "CastSpellByName" string
	log2 << "=== Searching for 'CastSpellByName' string ===" << std::endl;
	const unsigned char CastSpellByNamePattern[] = { 'C', 'a', 's', 't', 'S', 'p', 'e', 'l', 'l', 'B', 'y', 'N', 'a', 'm', 'e' };
	pointer castSpellRef = FindPatternSimple(WoWModuleInfo.lpBaseOfDll, WoWModuleInfo.SizeOfImage, CastSpellByNamePattern, sizeof(CastSpellByNamePattern));

	if (castSpellRef != nullptr)
	{
		DWORD castSpellAddr = reinterpret_cast<DWORD>(static_cast<void*>(castSpellRef));
		DWORD baseAddr = reinterpret_cast<DWORD>(WoWModuleInfo.lpBaseOfDll);
		DWORD offset = castSpellAddr - baseAddr;

		log2 << "Found 'CastSpellByName' at address: 0x" << std::hex << castSpellAddr << std::dec << std::endl;
		log2 << "Offset from base: 0x" << std::hex << offset << std::dec << std::endl;
		log2 << std::endl;

		// Dump 512 bytes before and after for analysis
		log2 << "=== Context around CastSpellByName (512 bytes before and after) ===" << std::endl;
		int start = (offset >= 512) ? offset - 512 : 0;
		int end = (offset + 512 + 15 < WoWModuleInfo.SizeOfImage) ? offset + 512 + 15 : WoWModuleInfo.SizeOfImage;

		for (int i = start; i < end; i++)
		{
			unsigned char byte = reinterpret_cast<unsigned char*>(baseAddr + i)[0];
			if (i == offset) log2 << "[";
			log2 << std::hex << std::setfill('0') << std::setw(2) << (int)byte << " ";
			if (i == offset + 14) log2 << "]";
			
			if ((i - start + 1) % 16 == 0)
				log2 << std::endl;
		}
		log2 << std::dec << std::endl << std::endl;
	}
	else
	{
		log2 << "ERROR: Could not find 'CastSpellByName' string in memory!" << std::endl;
	}

	log2.close();

	MessageBox(FindWindow(L"GxWindowClass", L"World of Warcraft"), 
		L"CastSpellByName search complete! Check C:\\WoW_Lua_Unlocker_Log.txt", 
		L"Debug Mode", MB_OK);

	return 1;
}
