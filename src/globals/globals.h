#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>
#include <string>
#include <vector>

#define APP_VERSION "5.0.20"

// Custom Window Messages
#define WM_TRAYICON           (WM_APP + 1)
#define WM_MPV_WAKEUP         (WM_APP + 2)
#define WM_RUN_UPDATER        (WM_APP + 3)
#define WM_REFRESH_WEBVIEW    (WM_APP + 4)
#define WM_AUTH_CODE_RECEIVED (WM_APP + 5)
#define WM_NAVIGATE_READY     (WM_APP + 6)
#define WM_POST_INITIALIZE    (WM_APP + 7)

// Tray Menu Item IDs
#define ID_TRAY_SHOWWINDOW        1001
#define ID_TRAY_ALWAYSONTOP       1002
#define ID_TRAY_CLOSE_ON_EXIT     1003
#define ID_TRAY_USE_DARK_THEME    1004
#define ID_TRAY_PAUSE_MINIMIZED   1005
#define ID_TRAY_PAUSE_FOCUS_LOST  1006
#define ID_TRAY_PICTURE_IN_PICTURE 1007
#define ID_TRAY_QUIT             1008

// Struct for the tray menu
struct MenuItem {
    UINT id;
    bool checked;
    bool separator;
    std::wstring text;
};

// Application-wide instance handle
extern HINSTANCE g_hInst;

// These URLs can be configured via command-line args
extern std::wstring g_webuiUrl;
extern std::vector<std::wstring> g_webuiUrls;

// Static application data (not user-configurable)
extern const std::vector<std::wstring> g_subtitleExtensions;

#endif // GLOBALS_H