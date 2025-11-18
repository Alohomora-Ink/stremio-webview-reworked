#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>

struct AppSettings {
    // General
    bool closeOnExit = false;
    bool useDarkTheme = true;
    bool pauseOnMinimize = true;
    bool pauseOnLostFocus = false;
    bool allowZoom = false;
    bool isRpcOn = true;
    bool alwaysOnTop = false;

    // MPV
    std::string initialVO = "gpu-next";
    int initialVolume = 50;
    
    // Window
    WINDOWPLACEMENT windowPlacement;
    bool hasSavedWindowPlacement = false;
};

class SettingsManager
{
public:
    SettingsManager();
    ~SettingsManager() = default;

    void Load();
    void Save();

    const AppSettings& GetSettings() const { return m_settings; }
    AppSettings& GetSettings() { return m_settings; }

private:
    std::wstring GetIniPath() const;
    void LoadWindowPlacement();
    void SaveWindowPlacement() const;

    AppSettings m_settings;
};

#endif // SETTINGS_MANAGER_H