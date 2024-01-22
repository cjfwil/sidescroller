#ifndef WIN32WINDOW_HPP
#define WIN32WINDOW_HPP

#include <windows.h>
#include <shellscalingapi.h>

class Win32Window
{
public:
    HINSTANCE hInstance = NULL;
    HWND hwnd = NULL;
    LPCSTR windowClassName = "Win32WindowClass";

    static LRESULT CALLBACK Win32Window::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static Win32Window &GetInstance()
    {
        static Win32Window instance;
        return (instance);
    }

private:        
    Win32Window()
    {        
        OutputDebugStringA("Creating Window.\n");

        HRESULT hr = CreateWin32Window();
        if (FAILED(hr)) {
            OutputDebugStringA("Failed to create D3D11 Window.\n");            
        }
    }

    HRESULT CreateWin32Window(char* title = "Win32 Window")
    {
        HRESULT hr = S_OK;
        hr = SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
        if (FAILED(hr))
            return (hr);

        if (hInstance == NULL)
            hInstance = (HINSTANCE)GetModuleHandle(NULL);

        HICON hIcon = NULL;
        char szExePath[MAX_PATH];
        DWORD len = GetModuleFileNameA(NULL, szExePath, MAX_PATH);
        if (len == 0)
            return HRESULT_FROM_WIN32(GetLastError());

        if (!hIcon)
            hIcon = ExtractIconA(hInstance, szExePath, 0);

        WNDCLASSA wndClass;
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = StaticWindowProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = hInstance;
        wndClass.hIcon = hIcon;
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = windowClassName;

        if (!RegisterClassA(&wndClass))
        {
            DWORD dwError = GetLastError();
            if (dwError != ERROR_CLASS_ALREADY_EXISTS)
                return HRESULT_FROM_WIN32(dwError);
        }

        RECT rc = {};
        int defaultWidth = 1024;
        int defaultHeight = 768;
        SetRect(&rc, 0, 0, defaultWidth, defaultHeight);
        HMENU hMenu = GetMenu(hwnd);
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, (hMenu != NULL) ? true : false);

        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        hwnd = CreateWindowA(windowClassName,
                             title,
                             WS_OVERLAPPEDWINDOW,
                             x, y,
                             (rc.right - rc.left), (rc.bottom - rc.top),
                             0,
                             hMenu,
                             hInstance,
                             0);

        if (hwnd == NULL)        
            return HRESULT_FROM_WIN32(GetLastError());        

        return (hr);
    }
};

LRESULT CALLBACK Win32Window::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
    {
        HMENU hMenu = GetMenu(hWnd);
        if (hMenu != NULL)
        {
            DestroyMenu(hMenu);
        }
        DestroyWindow(hWnd);
        UnregisterClassA(Win32Window::GetInstance().windowClassName, Win32Window::GetInstance().hInstance);
        return 0;
    }
    break;
    case WM_DESTROY:
    {
        PostQuitMessage(0);
    }
    break;
    }

    return (DefWindowProcA(hWnd, uMsg, wParam, lParam));
}

#endif