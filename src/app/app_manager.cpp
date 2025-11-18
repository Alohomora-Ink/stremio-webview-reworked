#include "app_manager.h"
#include "../logger/logger.h"
#include "../globals/globals.h"
#include "../window/window_manager.h"
#include "../mpv/mpv_manager.h"
#include "../webview/webview_manager.h"
#include "../server/server_manager.h"
#include "../discord/discord_manager.h"
#include "../updater/updater_manager.h"
#include "../extensions/extensions_manager.h"
#include "../webview_protocol/event_emitter/event_emitter.h"
#include "../helpers/helpers.h"
#include "../webview_protocol/transport_constants.h" 

AppManager::AppManager() : m_hMutex(nullptr) {}

AppManager::~AppManager() {
    if (m_settingsManager && m_windowManager) {
        WINDOWPLACEMENT wp = { sizeof(wp) };
        if (GetWindowPlacement(m_windowManager->GetHWND(), &wp)) {
            GetSettings().windowPlacement = wp;
            GetSettings().hasSavedWindowPlacement = true;
        }
        m_settingsManager->Save();
    }
    if (m_hMutex) CloseHandle(m_hMutex);
    Logger::Cleanup();
}

bool AppManager::InitializeWindow(int nCmdShow) {
    m_hMutex = CreateMutexW(nullptr, TRUE, L"StrematoSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return false; 
    m_settingsManager = std::make_unique<SettingsManager>();
    m_settingsManager->Load();
    m_commandHandler = std::make_unique<WebViewProtocol::CommandHandler>();
    m_windowManager = std::make_unique<WindowManager>(this);
    m_mpvManager = std::make_unique<MPVManager>(this);
    m_webviewManager = std::make_unique<WebViewManager>(this);
    m_serverManager = std::make_unique<ServerManager>();
    m_discordManager = std::make_unique<DiscordManager>(this);
    m_updaterManager = std::make_unique<UpdaterManager>(this);
    m_extensionsManager = std::make_unique<ExtensionsManager>(this);
    if (!m_windowManager->Create(nCmdShow)) return false;
    LOG_INFO("AppManager", "Window initialization complete.");
    return true;
}

bool AppManager::InitializeManagers() {
    HWND hWnd = m_windowManager->GetHWND();
    if (!hWnd) return false;
    if (!m_mpvManager->Initialize(hWnd) || !m_webviewManager->Initialize(hWnd)) return false;
    m_discordManager->Initialize();
    m_serverManager->Start();
    RegisterCommandHandlers();
    LOG_INFO("AppManager", "All managers initialized.");
    return true;
}

void AppManager::RegisterCommandHandlers() {
    using namespace WebViewProtocol; // Use the namespace for cleaner code

    m_commandHandler->RegisterCommand(Commands::PLAY, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->Play(payload.get<PlayPayload>());
    });

    m_commandHandler->RegisterCommand(Commands::STOP, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->Stop();
    });

    m_commandHandler->RegisterCommand(Commands::TOGGLE_PAUSE, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->TogglePause();
    });

    m_commandHandler->RegisterCommand(Commands::SEEK, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->Seek(payload.get<SeekPayload>());
    });

    m_commandHandler->RegisterCommand(Commands::SET_VOLUME, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->SetVolume(payload.get<SetVolumePayload>());
    });

    m_commandHandler->RegisterCommand(Commands::TOGGLE_MUTE, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->ToggleMute();
    });

    m_commandHandler->RegisterCommand(Commands::SET_PROPERTY, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->SetProperty(payload.get<SetPropertyPayload>());
    });

    m_commandHandler->RegisterCommand(Commands::LOAD_SUBTITLE, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_mpvManager->LoadSubtitle(payload.get<LoadSubtitlePayload>());
    });

    m_commandHandler->RegisterCommand(Commands::TOGGLE_FULLSCREEN, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_windowManager->ToggleFullScreen();
    });

    m_commandHandler->RegisterCommand(Commands::FRONTEND_READY, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_windowManager->OnFrontendReady();
    });

    m_commandHandler->RegisterCommand(Commands::SET_RPC, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_discordManager->SetPresence(payload.get<std::vector<std::string>>());
    });

    m_commandHandler->RegisterCommand(Commands::NAVIGATE, [this](const json& payload, const std::optional<std::string>& messageId) {
        auto url = Utf8ToWstring(payload.get<std::string>());
        m_webviewManager->Navigate(url);
    });

    m_commandHandler->RegisterCommand(Commands::UPDATE_INSTALL, [this](const json& payload, const std::optional<std::string>& messageId) {
        m_updaterManager->RunInstallerAndExit();
    });

    m_commandHandler->RegisterCommand(Commands::GET_SETTING, [this](const json& payload, const std::optional<std::string>& messageId) {
        if (!messageId) return;

        try {
            std::string key = payload.at("key").get<std::string>();
            json resultValue;

            if (key == "isAlwaysOnTop") {
                resultValue = m_settingsManager->GetSettings().alwaysOnTop;
            } else if (key == "isRpcOn") {
                resultValue = m_settingsManager->GetSettings().isRpcOn;
            } else {
                EventEmitter::emitCommandResponse(messageId.value(), std::nullopt, "Unknown setting key");
                return;
            }

            EventEmitter::emitCommandResponse(messageId.value(), resultValue, std::nullopt);
        } catch (const std::exception& e) {
            EventEmitter::emitCommandResponse(messageId.value(), std::nullopt, e.what());
        }
    });
}

int AppManager::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

WindowManager* AppManager::GetWindowManager() const { return m_windowManager.get(); }
MPVManager* AppManager::GetMPVManager() const { return m_mpvManager.get(); }
WebViewManager* AppManager::GetWebViewManager() const { return m_webviewManager.get(); }
UpdaterManager* AppManager::GetUpdaterManager() const { return m_updaterManager.get(); }
WebViewProtocol::CommandHandler* AppManager::GetCommandHandler() const { return m_commandHandler.get(); }
const AppSettings& AppManager::GetSettings() const { return m_settingsManager->GetSettings(); }
AppSettings& AppManager::GetSettings() { return m_settingsManager->GetSettings(); }
ExtensionsManager* AppManager::GetExtensionsManager() const { return m_extensionsManager.get(); }
ServerManager* AppManager::GetServerManager() const { return m_serverManager.get(); }