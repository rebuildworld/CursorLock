// CursorLock.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "CursorLock.h"

#include <tlhelp32.h>
#include <string>

#include "Hook.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
TCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

DWORD GetProcessIdByName(const std::string& name)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32 pe = {};
	pe.dwSize = sizeof(pe);
	DWORD pid = 0;
	BOOL ret = Process32First(snapshot, &pe);
	while (ret)
	{
		if (pe.szExeFile == name)
		{
			pid = pe.th32ProcessID;
			break;
		}
		ret = Process32Next(snapshot, &pe);
	}
	CloseHandle(snapshot);
	return pid;
}

struct EnumInfo
{
	DWORD pid;
	HWND window;
};

BOOL CALLBACK EnumWindowProc(HWND window, LPARAM lParam)
{
	EnumInfo* ei = reinterpret_cast<EnumInfo*>(lParam);
	DWORD pid = 0;
	GetWindowThreadProcessId(window, &pid);
	if (pid == ei->pid)
	{
		if (GetParent(window) == nullptr && IsWindowVisible(window))
		{
			ei->window = window;
			return FALSE;
		}
	}
	return TRUE;
}

HWND GetProcessMainWindow(DWORD pid)
{
	EnumInfo ei = {};
	ei.pid = pid;
	ei.window = nullptr;
	EnumWindows(&EnumWindowProc, reinterpret_cast<LPARAM>(&ei));
	return ei.window;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此处放置代码。

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CURSORLOCK, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CURSORLOCK));

	TCHAR procNameBuf[MAX_PATH] = {};
	DWORD ret = GetPrivateProfileString(_T("CursorLock"), _T("procName"), _T(""),
		procNameBuf, MAX_PATH - 1, _T(".\\CursorLock.ini"));
	TCHAR visibleBuf[MAX_PATH] = {};
	ret = GetPrivateProfileString(_T("CursorLock"), _T("visible"), _T("1"),
		visibleBuf, MAX_PATH - 1, _T(".\\CursorLock.ini"));
	bool visible = _tcscmp(visibleBuf, _T("0")) == 0 ? false : true;

	DWORD pid = GetProcessIdByName(procNameBuf);
	if (pid == 0)
	{
		MessageBox(nullptr, _T("Process not found."), _T("CursorLock"), MB_OK);
		return 1;
	}
	HWND window = GetProcessMainWindow(pid);
	if (window == nullptr)
	{
		MessageBox(nullptr, _T("Window not found."), _T("CursorLock"), MB_OK);
		return 1;
	}
	if (!Hook(window, visible))
	{
		MessageBox(nullptr, _T("Hook failed."), _T("CursorLock"), MB_OK);
		return 1;
	}

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Unhook();

	return (int)msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CURSORLOCK));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_CURSORLOCK);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
