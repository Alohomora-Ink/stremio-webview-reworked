#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <windows.h>
#include <memory>
#include "../webview_protocol/command_handler/command_handler.h"
#include "../settings/settings_manager.h" 
#include "../server/server_manager.h" 

class WindowManager;
class MPVManager;
class WebViewManager;
class DiscordManager;
class UpdaterManager;
class ExtensionsManager;

class AppManager
{
public:
    AppManager();
    ~AppManager();

    bool InitializeWindow(int nCmdShow); 
    bool InitializeManagers(); 
    int RunMessageLoop();

    WindowManager* GetWindowManager() const;
    MPVManager* GetMPVManager() const;
    WebViewManager* GetWebViewManager() const;
    UpdaterManager* GetUpdaterManager() const; 
    ServerManager* GetServerManager() const;

    WebViewProtocol::CommandHandler* GetCommandHandler() const;
    const AppSettings& GetSettings() const;
    AppSettings& GetSettings();
    ExtensionsManager* GetExtensionsManager() const;

private:
    void RegisterCommandHandlers();

    std::unique_ptr<SettingsManager> m_settingsManager;
    std::unique_ptr<WindowManager> m_windowManager;
    std::unique_ptr<MPVManager> m_mpvManager;
    std::unique_ptr<WebViewManager> m_webviewManager;
    std::unique_ptr<ServerManager> m_serverManager;
    std::unique_ptr<DiscordManager> m_discordManager;
    std::unique_ptr<UpdaterManager> m_updaterManager;
    std::unique_ptr<ExtensionsManager> m_extensionsManager;
    std::unique_ptr<WebViewProtocol::CommandHandler> m_commandHandler;
    HANDLE m_hMutex;
};

#endif // APP_MANAGER_H