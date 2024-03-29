#include "injector.h"

void hook::Patch(byte* dst, byte* src, size_t size) {
	memcpy(dst, src, size);
}

void hook::Nop(byte* dst, size_t size) {
	memset(dst, 0x90, size);
}

void hook::PatchEx(HANDLE hProc, byte* dst, byte* src, size_t size)
{
	DWORD oldProtect;
	VirtualProtectEx(hProc, dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	WriteProcessMemory(hProc, dst, src, size, nullptr);
	VirtualProtectEx(hProc, dst, size, oldProtect, &oldProtect);
}

void hook::NopEx(HANDLE hProc, byte* dst, size_t size) {
	byte* src = new byte[size];
	memset(src, 0x90, size);
	hook::PatchEx(hProc, dst, src, size);
	delete[] src;
}
#ifndef _WIN64
void hook::Detour(void* dst, void* func, const size_t size) {
	if (size < 5) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 5", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	hook::Nop((byte*)dst, size);
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	*(byte*)dst = 0xE9;
	*(uintptr_t*)((uintptr_t)dst + 1) = (uintptr_t)func - (uintptr_t)dst - 5;
	VirtualProtect(dst, size, oldProtect, &oldProtect);
}
#else
void hook::Detour(void* dst, void* func, const size_t size) {
	if (size < 13) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 13", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	hook::Nop((byte*)dst, size);
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	byte buf[] = "\x50\x48\xB8\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xFF\xE0"; // Need "Pop rax"
	*(uintptr_t*)((uintptr_t)buf + 3) = (uintptr_t)func - (uintptr_t)dst - 13;
	memcpy(dst, buf, 13);
	VirtualProtect(dst, size, oldProtect, &oldProtect);
}
#endif

#ifndef _WIN64
void hook::DetourEx(HANDLE hProc, void* dst, void* func, const size_t size) {
	if (size < 5) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 5", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	hook::NopEx(hProc, (byte*)dst, size);
	DWORD oldProtect;
	VirtualProtectEx(hProc, dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	byte buf[sizeof(uintptr_t) + 1];
	
	*(byte*)buf = 0xE9;
	*(uintptr_t*)((uintptr_t)buf + 1) = (uintptr_t)func - (uintptr_t)dst - 5;

	WriteProcessMemory(hProc, dst, (void*)buf, sizeof(uintptr_t) + 1, nullptr);
	VirtualProtectEx(hProc, dst, size, oldProtect, &oldProtect);
}
#else
void hook::DetourEx(HANDLE hProc, void* dst, void* func, const size_t size) {
	if (size < 13) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 13", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	hook::NopEx(hProc, (byte*)dst, size);
	DWORD oldProtect;
	VirtualProtectEx(hProc, dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	byte buf[] = "\x50\x48\xB8\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xFF\xE0";
	*(uintptr_t*)((uintptr_t)buf + 3) = (uintptr_t)func - (uintptr_t)dst - 13;

	WriteProcessMemory(hProc, dst, buf, 13, nullptr); // Need "Pop rax"
	VirtualProtectEx(hProc, dst, size, oldProtect, &oldProtect);
}
#endif

#ifndef _WIN64
uintptr_t hook::TrampHook(void* dst, void* func, size_t size) {
	if (size < 5) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 5", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	uintptr_t gateway = (uintptr_t)VirtualAlloc(NULL, size + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); // Restore stolen bytes.
	memcpy((void*)gateway, dst, size);
	hook::Detour(dst, (void*)gateway, size);
	hook::Detour((void*)(gateway + size), func, 5);
	return gateway;
}
uintptr_t hook::TrampHookEx(HANDLE hProc, void* dst, void* func, size_t size) {
	if (size < 5) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 5", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	uintptr_t gateway = (uintptr_t)VirtualAllocEx(hProc, NULL, size + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); // Restore stolen bytes.
	memcpy((void*)gateway, dst, size);
	hook::DetourEx(hProc, dst, (void*)gateway, size);
	hook::DetourEx(hProc, (void*)(gateway + size), func, 5);
	return gateway;
}
#else
uintptr_t hook::TrampHook(void* dst, void* func, size_t size) {
	if (size < 13) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 13", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	uintptr_t gateway = (uintptr_t)VirtualAlloc(NULL, size + 14, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); // Restore stolen bytes.
	*(byte*)gateway = 0x58; // pop rax
	memcpy((void*)(gateway + 1), dst, size);
	hook::Detour(dst, (void*)(gateway + 1), size);
	hook::Detour((void*)(gateway + size + 1), func, 13);
	return gateway;
}
uintptr_t hook::TrampHookEx(HANDLE hProc, void* dst, void* func, size_t size) {
	if (size < 13) {
#ifdef _DEBUG
		MessageBoxW(NULL, L"Error in detour: your buffer size could not less than 13", L"ERROR", MB_OK | MB_ICONERROR);
#endif
		return;
	}
	uintptr_t gateway = (uintptr_t)VirtualAllocEx(hProc, NULL, size + 14, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); // Restore stolen bytes.
	byte opcode = 0x58; //pop rax
	WriteProcessMemory(hProc, (void*)gateway, &opcode, 1, nullptr); // Write pop rax to gateway head.
	memcpy((void*)(gateway + 1), dst, size);
	hook::DetourEx(hProc, dst, (void*)(gateway + 1), size);
	hook::DetourEx(hProc, (void*)(gateway + size + 1), func, 13);
	return gateway;
}
#endif