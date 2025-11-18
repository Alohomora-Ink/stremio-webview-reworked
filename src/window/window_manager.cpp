#include "window_manager.h"
#include "../app/app_manager.h"
#include "../logger/logger.h"
#include "../resource.h"
#include "../helpers/helpers.h" 
#include "../mpv/mpv_manager.h"
#include "../webview/webview_manager.h"
#include "../updater/updater_manager.h"
#include <windowsx.h>
#include <gdiplus.h>
#include <dwmapi.h>

// UI constants
static const int TRAY_ITEM_HEIGHT = 28;
static const int TRAY_SEP_HEIGHT = 8;
static const int TRAY_WIDTH = 200;
static const int FONT_HEIGHT = 12;

// Static members
WindowManager* WindowManager::s_trayInstance = nullptr;
float WindowManager::s_splashOpacity = 1.0f;
int WindowManager::s_pulseDirection = -1;
int WindowManager::s_hoverIndex = -1;
HWND WindowManager::s_trayHwnd = nullptr;
std::vector<MenuItem> WindowManager::s_menuItems;

WindowManager::WindowManager(AppManager* appManager)
    : m_appManager(appManager), m_hWnd(nullptr), m_isFullscreen(false), m_isPipMode(false), m_isWindowVisible(true),
      m_darkBrush(nullptr), m_hSplash(nullptr), m_hSplashImage(nullptr), m_gdiplusToken(0), m_hMenuFont(nullptr)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
    Gdiplus::GdiplusStartupInput gpsi;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gpsi, nullptr);
    s_trayInstance = this;
}

WindowManager::~WindowManager() {
    RemoveTrayIcon();
    if (m_darkBrush) DeleteObject(m_darkBrush);
    if (m_hMenuFont) DeleteObject(m_hMenuFont);
    if (m_gdiplusToken) Gdiplus::GdiplusShutdown(m_gdiplusToken);
    s_trayInstance = nullptr;
}

bool WindowManager::Create(int nCmdShow) {
    m_darkBrush = CreateSolidBrush(RGB(12, 11, 17));
    WNDCLASSEXW wcex = { sizeof(wcex), CS_HREDRAW | CS_VREDRAW, WindowManager::StaticWndProc, 0, 0, g_hInst, nullptr, LoadCursor(nullptr, IDC_ARROW), m_darkBrush, nullptr, L"StrematoWindowClass", LoadIconW(g_hInst, MAKEINTRESOURCEW(IDR_MAINFRAME)) };
    if (!RegisterClassExW(&wcex)) {
        LOG_ERROR("WindowManager", "RegisterClassEx failed!");
        return false;
    }

    m_hWnd = CreateWindowExW(0, L"StrematoWindowClass", L"Stremato", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 900, nullptr, nullptr, g_hInst, this);
    if (!m_hWnd) {
        LOG_ERROR("WindowManager", "CreateWindowEx failed!");
        return false;
    }

    LoadCustomMenuFont();
    
    const auto& settings = m_appManager->GetSettings();
    if (settings.hasSavedWindowPlacement) {
        SetWindowPlacement(m_hWnd, &settings.windowPlacement);
        ShowWindow(m_hWnd, settings.windowPlacement.showCmd);
    } else {
        ShowWindow(m_hWnd, nCmdShow);
    }
    UpdateWindow(m_hWnd);

    return true;
}

void WindowManager::UpdateTheme() {
    #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
    BOOL useDark = m_appManager->GetSettings().useDarkTheme;
    DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
}

LRESULT CALLBACK WindowManager::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    WindowManager* pThis = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (WindowManager*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;
    } else {
        pThis = (WindowManager*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }
    return pThis ? pThis->MemberWndProc(hWnd, message, wParam, lParam) : DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT WindowManager::MemberWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    const auto& settings = m_appManager->GetSettings();
    auto& mutableSettings = m_appManager->GetSettings();

    switch (message) {
        
    case WM_CREATE:
        CreateTrayIcon();
        UpdateTheme();
        CreateSplashScreen();
        if(settings.alwaysOnTop) SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_POST_INITIALIZE:
    {
        if (m_appManager) {
            m_appManager->InitializeManagers();
        }
        return 0;
    }
    case WM_NAVIGATE_READY: {
        auto* url = reinterpret_cast<std::wstring*>(lParam);
        if (url) {
            LOG_INFO("WindowManager", "Received navigation request for: " + WStringToUtf8(*url));
            m_appManager->GetWebViewManager()->Navigate(*url);
            delete url;
        }
        return 0;
    }
    case WM_SIZE:
        if (m_appManager->GetWebViewManager()) m_appManager->GetWebViewManager()->Resize();
        if (m_hSplash) SetWindowPos(m_hSplash, nullptr, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER);
        if (wParam == SIZE_MINIMIZED && settings.pauseOnMinimize) m_appManager->GetMPVManager()->Pause();
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && settings.pauseOnLostFocus) m_appManager->GetMPVManager()->Pause();
        break;
    case WM_MPV_WAKEUP:
        m_appManager->GetMPVManager()->HandleEvents();
        break;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) ShowTrayMenu();
        if (lParam == WM_LBUTTONDBLCLK) { ShowWindow(hWnd, SW_RESTORE); SetForegroundWindow(hWnd); m_isWindowVisible = true; }
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
            case ID_TRAY_SHOWWINDOW:
                m_isWindowVisible = !m_isWindowVisible;
                ShowWindow(hWnd, m_isWindowVisible ? SW_SHOW : SW_HIDE);
                break;
            case ID_TRAY_ALWAYSONTOP:
                mutableSettings.alwaysOnTop = !settings.alwaysOnTop;
                SetWindowPos(hWnd, mutableSettings.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
                break;
            case ID_TRAY_CLOSE_ON_EXIT: mutableSettings.closeOnExit = !settings.closeOnExit; break;
            case ID_TRAY_USE_DARK_THEME: mutableSettings.useDarkTheme = !settings.useDarkTheme; UpdateTheme(); break;
            case ID_TRAY_PICTURE_IN_PICTURE: TogglePictureInPicture(!m_isPipMode); break;
            case ID_TRAY_PAUSE_FOCUS_LOST: mutableSettings.pauseOnLostFocus = !settings.pauseOnLostFocus; break;
            case ID_TRAY_PAUSE_MINIMIZED: mutableSettings.pauseOnMinimize = !settings.pauseOnMinimize; break;
            case ID_TRAY_QUIT: DestroyWindow(hWnd); break;
        }
        break;
    case WM_CLOSE:
        if (settings.closeOnExit) {
            DestroyWindow(hWnd);
        } else {
            ShowWindow(hWnd, SW_HIDE);
            if (settings.pauseOnMinimize) m_appManager->GetMPVManager()->Pause();
            m_isWindowVisible = false;
        }
        break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


void WindowManager::ShowTrayMenu()
{
    const auto& settings = m_appManager->GetSettings();
    s_menuItems.clear();
    s_menuItems.push_back({ ID_TRAY_SHOWWINDOW, m_isWindowVisible, false, L"Show Window" });
    s_menuItems.push_back({ ID_TRAY_ALWAYSONTOP, settings.alwaysOnTop, false, L"Always on Top" });
    s_menuItems.push_back({ ID_TRAY_PICTURE_IN_PICTURE, m_isPipMode, false, L"Picture in Picture" });
    s_menuItems.push_back({ ID_TRAY_PAUSE_MINIMIZED, settings.pauseOnMinimize, false, L"Pause Minimized" });
    s_menuItems.push_back({ ID_TRAY_PAUSE_FOCUS_LOST, settings.pauseOnLostFocus, false, L"Pause Unfocused" });
    s_menuItems.push_back({ ID_TRAY_CLOSE_ON_EXIT, settings.closeOnExit, false, L"Close on Exit" });
    s_menuItems.push_back({ ID_TRAY_USE_DARK_THEME, settings.useDarkTheme, false, L"Use Dark Theme" });
    s_menuItems.push_back({ 0, false, true, L"" });
    s_menuItems.push_back({ ID_TRAY_QUIT, false, false, L"Quit" });

    HWND hMenuWnd = CreateDarkTrayMenuWindow();
    int totalH = 0;
    for (const auto &it : s_menuItems) {
        totalH += it.separator ? TRAY_SEP_HEIGHT : TRAY_ITEM_HEIGHT;
    }

    POINT pt;
    GetCursorPos(&pt);
    SetWindowPos(hMenuWnd, HWND_TOPMOST, pt.x - TRAY_WIDTH, pt.y - totalH, TRAY_WIDTH, totalH, SWP_SHOWWINDOW);
    SetForegroundWindow(hMenuWnd);
}

void WindowManager::OnFrontendReady()
{
    HideSplashScreen();
    LOG_INFO("WindowManager", "Frontend is ready. Hiding splash screen.");
}
void WindowManager::ToggleFullScreen()
{
    static WINDOWPLACEMENT prevPlc = { sizeof(prevPlc) };
    m_isFullscreen = !m_isFullscreen;

    if (m_isFullscreen) {
        GetWindowPlacement(m_hWnd, &prevPlc);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLongW(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(m_hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    } else {
        SetWindowLongW(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPlacement(m_hWnd, &prevPlc);
    }
}
void WindowManager::CreateSplashScreen()
{
    WNDCLASSEXW splashWcex = {0};
    splashWcex.cbSize = sizeof(WNDCLASSEXW);
    splashWcex.lpfnWndProc = WindowManager::SplashWndProc;
    splashWcex.hInstance = g_hInst;
    splashWcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    splashWcex.lpszClassName = L"SplashScreenClass";
    RegisterClassExW(&splashWcex);

    RECT rcClient; GetClientRect(m_hWnd, &rcClient);
    m_hSplash = CreateWindowExW(0, L"SplashScreenClass", nullptr, WS_CHILD | WS_VISIBLE,
        0, 0, rcClient.right, rcClient.bottom, m_hWnd, nullptr, g_hInst, this);

    HRSRC hRes = FindResource(g_hInst, MAKEINTRESOURCE(IDR_SPLASH_PNG), RT_RCDATA);
    if(hRes) {
        HGLOBAL hData = LoadResource(g_hInst, hRes);
        void* pData = LockResource(hData);
        DWORD size = SizeofResource(g_hInst, hRes);
        IStream* pStream = nullptr;
        if(CreateStreamOnHGlobal(nullptr, TRUE, &pStream) == S_OK) {
            pStream->Write(pData, size, nullptr);
            Gdiplus::Bitmap bitmap(pStream);
            if(bitmap.GetLastStatus() == Gdiplus::Ok) {
                bitmap.GetHBITMAP(Gdiplus::Color(0,0,0,0), &m_hSplashImage);
            }
            pStream->Release();
        }
    }
    SetTimer(m_hSplash, 1, 16, nullptr);
    ShowWindow(m_hSplash, SW_SHOW);
}
void WindowManager::HideSplashScreen()
{
    if (m_hSplash) {
        KillTimer(m_hSplash, 1);
        DestroyWindow(m_hSplash);
        m_hSplash = nullptr;
    }
    if (m_hSplashImage) {
        DeleteObject(m_hSplashImage);
        m_hSplashImage = nullptr;
    }
}
LRESULT CALLBACK WindowManager::SplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowManager* pThis = (WindowManager*)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);

    switch(message)
    {
    case WM_TIMER: {
        s_splashOpacity += 0.01f * s_pulseDirection;
        if(s_splashOpacity <= 0.3f) { s_splashOpacity = 0.3f; s_pulseDirection = 1; }
        else if(s_splashOpacity >= 1.0f) { s_splashOpacity = 1.0f; s_pulseDirection = -1; }
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(memDC, memBmp);

        FillRect(memDC, &rc, CreateSolidBrush(RGB(12, 11, 17)));

        if(pThis && pThis->m_hSplashImage) {
            BITMAP bm; GetObject(pThis->m_hSplashImage, sizeof(bm), &bm);
            HDC imgDC = CreateCompatibleDC(memDC);
            SelectObject(imgDC, pThis->m_hSplashImage);
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(s_splashOpacity * 255), AC_SRC_ALPHA };
            AlphaBlend(memDC, (rc.right - bm.bmWidth)/2, (rc.bottom - bm.bmHeight)/2, bm.bmWidth, bm.bmHeight,
                       imgDC, 0, 0, bm.bmWidth, bm.bmHeight, bf);
            DeleteDC(imgDC);
        }

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
        DeleteDC(memDC);
        DeleteObject(memBmp);
        EndPaint(hWnd, &ps);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
void WindowManager::CreateTrayIcon()
{
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
    wcscpy_s(m_nid.szTip, L"Stremato");
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}
void WindowManager::RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
    if(m_nid.hIcon) DestroyIcon(m_nid.hIcon);
}
void WindowManager::LoadCustomMenuFont()
{
    if (m_hMenuFont) DeleteObject(m_hMenuFont);
    LOGFONTW lf = { -FONT_HEIGHT, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, CLEARTYPE_QUALITY, 0, L"Arial Rounded MT" };
    m_hMenuFont = CreateFontIndirectW(&lf);
}
HWND WindowManager::CreateDarkTrayMenuWindow()
{
    WNDCLASSEXW wcex = { sizeof(wcex), CS_HREDRAW | CS_VREDRAW, DarkTrayMenuProc, 0, 0, g_hInst, nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"DarkTrayMenuWnd", nullptr };
    RegisterClassExW(&wcex);
    s_trayHwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"DarkTrayMenuWnd", L"", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, g_hInst, nullptr);
    return s_trayHwnd;
}
void WindowManager::TogglePictureInPicture(bool enable)
{
    m_isPipMode = enable;
    m_appManager->GetSettings().alwaysOnTop = enable;
    LONG_PTR style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
    if(enable) style &= ~WS_CAPTION;
    else style |= WS_CAPTION;
    SetWindowLongPtr(m_hWnd, GWL_STYLE, style);
    SetWindowPos(m_hWnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_FRAMECHANGED);
}
LRESULT CALLBACK WindowManager::DarkTrayMenuProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WindowManager* pThis = WindowManager::s_trayInstance;
    if (!pThis) return DefWindowProc(hWnd, msg, wParam, lParam);
    const auto& settings = pThis->m_appManager->GetSettings();

    switch(msg) {
        case WM_CREATE: SetCapture(hWnd); break;
        case WM_KILLFOCUS: DestroyWindow(hWnd); break;
        case WM_LBUTTONUP: {
            if(s_hoverIndex != -1 && static_cast<size_t>(s_hoverIndex) < s_menuItems.size() && !s_menuItems[s_hoverIndex].separator) {
                PostMessage(pThis->m_hWnd, WM_COMMAND, s_menuItems[s_hoverIndex].id, 0);
            }
            DestroyWindow(hWnd);
            break;
        }
        case WM_DESTROY:
            s_hoverIndex = -1;
            ReleaseCapture();
            s_trayHwnd = nullptr;
            break;
        case WM_MOUSEMOVE: {
            int yPos = GET_Y_LPARAM(lParam);
            int curY = 0, newHover = -1;
            for(size_t i = 0; i < s_menuItems.size(); ++i) {
                int h = s_menuItems[i].separator ? TRAY_SEP_HEIGHT : TRAY_ITEM_HEIGHT;
                if(yPos >= curY && yPos < curY + h) { newHover = static_cast<int>(i); break; }
                curY += h;
            }
            if(newHover != s_hoverIndex) { s_hoverIndex = newHover; InvalidateRect(hWnd, NULL, TRUE); }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc; GetClientRect(hWnd, &rc);
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBmp);

            COLORREF bgBase = settings.useDarkTheme ? RGB(30,30,30) : RGB(240,240,240);
            COLORREF bgHover = settings.useDarkTheme ? RGB(50,50,50) : RGB(200,200,200);
            COLORREF txtNormal = settings.useDarkTheme ? RGB(200,200,200) : RGB(0,0,0);
            
            HBRUSH hBgBrush = CreateSolidBrush(bgBase);
            HBRUSH hHoverBrush = CreateSolidBrush(bgHover);
            FillRect(memDC, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, txtNormal);
            SelectObject(memDC, pThis->m_hMenuFont);

            int y = 0;
            for(size_t i = 0; i < s_menuItems.size(); ++i) {
                if (s_menuItems[i].separator) { y += TRAY_SEP_HEIGHT; continue; }
                RECT itemRc = {0, y, rc.right, y + TRAY_ITEM_HEIGHT};
                if(static_cast<int>(i) == s_hoverIndex) FillRect(memDC, &itemRc, hHoverBrush);
                if(s_menuItems[i].checked) TextOutW(memDC, 5, y + 7, L"✔", 1);
                TextOutW(memDC, 24, y + 7, s_menuItems[i].text.c_str(), (int)s_menuItems[i].text.length());
                y += TRAY_ITEM_HEIGHT;
            }
            DeleteObject(hHoverBrush);
            
            BitBlt(hdc, 0,0,rc.right,rc.bottom,memDC,0,0,SRCCOPY);
            DeleteDC(memDC);
            DeleteObject(memBmp);
            EndPaint(hWnd, &ps);
            break;
        }
        default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}