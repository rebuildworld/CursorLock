#include "framework.h"
#include "dllmain.h"
#include "Hook.h"

#pragma data_seg("HookShared")
HOOK_API HHOOK g_hook = nullptr;
HOOK_API HWND g_window = nullptr;
HOOK_API bool g_visible = true;
#pragma data_seg()
#pragma comment(linker, "/section:HookShared,rws")

RECT g_rcClip = {};
RECT g_rcOldClip = {};
HCURSOR g_oldCursor = nullptr;

LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code < 0)
		return CallNextHookEx(g_hook, code, wParam, lParam);

	switch (code)
	{
	case HC_ACTION:
		CWPSTRUCT* cwp = reinterpret_cast<CWPSTRUCT*>(lParam);
		if (cwp != nullptr && cwp->hwnd == g_window)
		{
			switch (cwp->message)
			{
			case WM_ACTIVATE:
				switch (LOWORD(cwp->wParam))
				{
				case WA_ACTIVE:
				case WA_CLICKACTIVE:
					SetCapture(cwp->hwnd);
					//GetClipCursor(&g_rcOldClip);
					GetClientRect(cwp->hwnd, &g_rcClip);
					ClientToScreen(cwp->hwnd, reinterpret_cast<POINT*>(&g_rcClip.left));
					ClientToScreen(cwp->hwnd, reinterpret_cast<POINT*>(&g_rcClip.right));
					ClipCursor(&g_rcClip);
					if (!g_visible)
						g_oldCursor = SetCursor(nullptr);
					break;

				case WA_INACTIVE:
					if (!g_visible)
						SetCursor(g_oldCursor);
					//ClipCursor(&g_rcOldClip);
					ClipCursor(nullptr);
					ReleaseCapture();
					break;
				}
				break;
			}
		}
		break;
	}

	return CallNextHookEx(g_hook, code, wParam, lParam);
}

bool WINAPI Hook(HWND window, bool visible)
{
	DWORD threadId = GetWindowThreadProcessId(window, nullptr);
	g_hook = SetWindowsHookEx(WH_CALLWNDPROC, &HookProc, g_module, threadId);
	if (g_hook == nullptr)
		return false;
	g_window = window;
	g_visible = visible;
	return true;
}

void WINAPI Unhook()
{
	if (g_hook != nullptr)
	{
		UnhookWindowsHookEx(g_hook);
		g_hook = nullptr;
		g_window = nullptr;
		g_visible = true;
	}
}
