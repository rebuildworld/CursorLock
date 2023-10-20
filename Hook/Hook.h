#pragma once

#ifdef HOOK_EXPORTS
#define HOOK_API extern "C" __declspec(dllexport)
#else
#define HOOK_API extern "C" __declspec(dllimport)
#endif

HOOK_API bool WINAPI Hook(HWND window, bool visible);
HOOK_API void WINAPI Unhook();
