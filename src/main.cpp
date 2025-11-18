#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#include <windows.h>
#include "globals/globals.h"
#include "app/app_manager.h"
#include "logger/logger.h"
#include "helpers/helpers.h"
#include "window/window_manager.h" 

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Logger::Init(GetExeDirectory() + L"\\portable_config");
    
    g_hInst = hInstance;

    AppManager app;
    if (!app.InitializeWindow(nCmdShow))
    {
        Logger::Cleanup();
        return 1;
    }
    HWND hWnd = app.GetWindowManager()->GetHWND();
    if (hWnd) {
        PostMessage(hWnd, WM_POST_INITIALIZE, 0, 0);
    }

    int result = app.RunMessageLoop();

    return result;
}