#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>
#include "../globals/globals.h"

class AppManager;

class WindowManager
{
public:
    explicit WindowManager(AppManager* appManager);
    ~WindowManager();

    bool Create(int nCmdShow);
    HWND GetHWND() const { return m_hWnd; }

    void ToggleFullScreen();
    void OnFrontendReady();
    void UpdateTheme();
    bool IsFullScreen() const { return m_isFullscreen; }

private:
    // Window Procedure
    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT MemberWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Splash Screen
    void CreateSplashScreen();
    void HideSplashScreen();
    static LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Tray Icon
    void CreateTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void LoadCustomMenuFont();
    void TogglePictureInPicture(bool enable);
    HWND CreateDarkTrayMenuWindow();
    static LRESULT CALLBACK DarkTrayMenuProc(HWND, UINT, WPARAM, LPARAM);

    AppManager* m_appManager;
    HWND m_hWnd;
    bool m_isFullscreen;
    bool m_isPipMode;
    bool m_isWindowVisible;

    HBRUSH m_darkBrush;
    static WindowManager* s_trayInstance;

    HWND m_hSplash;
    HBITMAP m_hSplashImage;
    ULONG_PTR m_gdiplusToken;
    static float s_splashOpacity;
    static int s_pulseDirection;

    NOTIFYICONDATAW m_nid;
    HFONT m_hMenuFont;
    static int s_hoverIndex;
    static HWND s_trayHwnd;
    static std::vector<MenuItem> s_menuItems;
};

#endif // WINDOW_MANAGER_H