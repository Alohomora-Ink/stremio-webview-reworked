#include "settings_manager.h"
#include "../helpers/helpers.h"
#include <sstream>

SettingsManager::SettingsManager()
{
    m_settings.windowPlacement.length = sizeof(WINDOWPLACEMENT);
}

std::wstring SettingsManager::GetIniPath() const
{
    std::wstring exeDir = GetExeDirectory();
    std::wstring pcDir = exeDir + L"\\portable_config";
    CreateDirectoryW(pcDir.c_str(), nullptr);
    return pcDir + L"\\Stremato-settings.ini";
}

void SettingsManager::Load()
{
    std::wstring iniPath = GetIniPath();
    wchar_t buffer[32];

    m_settings.closeOnExit = (GetPrivateProfileIntW(L"General", L"CloseOnExit", 0, iniPath.c_str()) == 1);
    m_settings.useDarkTheme = (GetPrivateProfileIntW(L"General", L"UseDarkTheme", 1, iniPath.c_str()) == 1);
    m_settings.pauseOnMinimize = (GetPrivateProfileIntW(L"General", L"PauseOnMinimize", 1, iniPath.c_str()) == 1);
    m_settings.pauseOnLostFocus = (GetPrivateProfileIntW(L"General", L"PauseOnLostFocus", 0, iniPath.c_str()) == 1);
    m_settings.allowZoom = (GetPrivateProfileIntW(L"General", L"AllowZoom", 0, iniPath.c_str()) == 1);
    m_settings.isRpcOn = (GetPrivateProfileIntW(L"General", L"DiscordRPC", 1, iniPath.c_str()) == 1);
    m_settings.alwaysOnTop = (GetPrivateProfileIntW(L"General", L"AlwaysOnTop", 0, iniPath.c_str()) == 1);
    
    m_settings.initialVolume = GetPrivateProfileIntW(L"MPV", L"InitialVolume", 50, iniPath.c_str());
    GetPrivateProfileStringW(L"MPV", L"VideoOutput", L"gpu-next", buffer, _countof(buffer), iniPath.c_str());
    m_settings.initialVO = WStringToUtf8(buffer);

    LoadWindowPlacement();
}

void SettingsManager::Save()
{
    std::wstring iniPath = GetIniPath();

    WritePrivateProfileStringW(L"General", L"CloseOnExit", m_settings.closeOnExit ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"UseDarkTheme", m_settings.useDarkTheme ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"PauseOnMinimize", m_settings.pauseOnMinimize ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"PauseOnLostFocus", m_settings.pauseOnLostFocus ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"AllowZoom", m_settings.allowZoom ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"DiscordRPC", m_settings.isRpcOn ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(L"General", L"AlwaysOnTop", m_settings.alwaysOnTop ? L"1" : L"0", iniPath.c_str());

    WritePrivateProfileStringW(L"MPV", L"InitialVolume", std::to_wstring(m_settings.initialVolume).c_str(), iniPath.c_str());

    SaveWindowPlacement();
}

void SettingsManager::LoadWindowPlacement()
{
    std::wstring iniPath = GetIniPath();
    m_settings.windowPlacement.showCmd = GetPrivateProfileIntW(L"Window", L"ShowCmd", SW_SHOWNORMAL, iniPath.c_str());
    m_settings.windowPlacement.rcNormalPosition.left = GetPrivateProfileIntW(L"Window", L"Left", 0, iniPath.c_str());
    m_settings.windowPlacement.rcNormalPosition.top = GetPrivateProfileIntW(L"Window", L"Top", 0, iniPath.c_str());
    m_settings.windowPlacement.rcNormalPosition.right = GetPrivateProfileIntW(L"Window", L"Right", 0, iniPath.c_str());
    m_settings.windowPlacement.rcNormalPosition.bottom = GetPrivateProfileIntW(L"Window", L"Bottom", 0, iniPath.c_str());

    if (m_settings.windowPlacement.rcNormalPosition.right > 0 && m_settings.windowPlacement.rcNormalPosition.bottom > 0) {
        m_settings.hasSavedWindowPlacement = true;
    }
}

void SettingsManager::SaveWindowPlacement() const
{
    if (!m_settings.hasSavedWindowPlacement) return;
    std::wstring iniPath = GetIniPath();
    const auto& wp = m_settings.windowPlacement;
    WritePrivateProfileStringW(L"Window", L"ShowCmd", std::to_wstring(wp.showCmd).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Left", std::to_wstring(wp.rcNormalPosition.left).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Top", std::to_wstring(wp.rcNormalPosition.top).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Right", std::to_wstring(wp.rcNormalPosition.right).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"Bottom", std::to_wstring(wp.rcNormalPosition.bottom).c_str(), iniPath.c_str());
}