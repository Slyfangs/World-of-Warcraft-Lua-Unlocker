#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>

#pragma comment (lib, "Psapi.lib")

#include "Pointer.hpp"

// 3.3.5a Pattern: Bypasses the insecure function check for protected Lua functions
// like CastSpellByName and CastSpellByID
// Pattern: 56 8B F1 8B 0D ? ? ? ? 8B 11 FF 92 ? ? ? ? 84 C0 74 10
// Mask:    xxxxx????xxxx????xxxx
const unsigned char NewBytes[] = { 0xEB, 0x10 };
const unsigned char PatternBytes[] = { 0x56, 0x8B, 0xF1, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x11, 0xFF, 0x92, 0x00, 0x00, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x10 };
const char PatternMask[] = "xxxxx????xxxx????xxxx";

pointer FindPattern(pointer StartAddress, unsigned int MaxLength, const unsigned char* Bytes, const char* Mask)
{
	unsigned int i = 0;
	while (i < MaxLength)
	{
		unsigned int o = 0;
		while (reinterpret_cast<unsigned char*>(StartAddress + i)[o] == Bytes[o] || Mask[o] == '*')
		{
			o++;
			if (Mask[o] == 0x0)
			{
				return StartAddress + i;
			}
		}

		i++;
	}

	return nullptr;
}

int __stdcall DllMain(void* Module, unsigned long Reason, void*)
{
	if (Reason != DLL_PROCESS_ATTACH)
	{
		return 0;
	}

	MODULEINFO WoWModuleInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &WoWModuleInfo, sizeof(MODULEINFO));
	pointer PatchAddress = FindPattern(WoWModuleInfo.lpBaseOfDll, WoWModuleInfo.SizeOfImage, PatternBytes, PatternMask);
	if (PatchAddress == nullptr)
	{
		MessageBox(FindWindow(L"GxWindowClass", L"World of Warcraft"), L"Unable to find bytes to patch.", L"Failure", MB_OK);
		return 0;
	}

	// Offset to the conditional jump (74 10) that needs to be patched
	// Pattern is 21 bytes long (0-20), and "74 10" is at the end (bytes 19-20)
	PatchAddress += 0x13;

	unsigned long OldProtection = 0;
	VirtualProtect(PatchAddress, 0x2, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(PatchAddress, NewBytes, 0x2);
	VirtualProtect(PatchAddress, 0x2, OldProtection, &OldProtection);

	if (memcmp(NewBytes, PatchAddress, 0x2) != 0)
	{
		wchar_t Message[100];
		ZeroMemory(Message, sizeof(Message));
		swprintf_s(Message, 100, L"Unable to patch bytes. Error: %d", GetLastError());
		MessageBox(FindWindow(L"GxWindowClass", L"World of Warcraft"), Message, L"Failure", MB_OK);
	}
	else
	{
		MessageBox(FindWindow(L"GxWindowClass", L"World of Warcraft"), L"Lua unlocked.", L"Success", MB_OK);
	}

	return 1;
}
