#include "discord_manager.h"
#include "../app/app_manager.h"
#include "../logger/logger.h"
#include <chrono>

const char* DISCORD_CLIENT_ID = "1438324305581834453";

DiscordManager::DiscordManager(AppManager* appManager) : m_appManager(appManager) {} 

DiscordManager::~DiscordManager() {
    discord::RPCManager::get().shutdown();
}

void DiscordManager::Initialize() {
    if (!m_appManager->GetSettings().isRpcOn) return; 
    
    auto& manager = discord::RPCManager::get();

    manager.onReady([](discord::User const& user) {
        LOG_INFO("DiscordManager", "Connected to Discord user: " + user.username);
    });
    manager.onDisconnected([](int errCode, std::string_view msg) {
        LOG_WARN("DiscordManager", "Disconnected (" + std::to_string(errCode) + "): " + std::string(msg));
    });
    manager.onErrored([](int errCode, std::string_view msg) {
        LOG_ERROR("DiscordManager", "Error (" + std::to_string(errCode) + "): " + std::string(msg));
    });

    manager.setClientID(DISCORD_CLIENT_ID);
    manager.initialize();
}

void DiscordManager::SetPresence(const std::vector<std::string>& args) {
    if (!m_appManager->GetSettings().isRpcOn || args.empty()) { 
        discord::RPCManager::get().clearPresence();
        return;
    }

    const std::string& activityType = args[0];
    
    if (activityType == "watching" && args.size() >= 11) {
        SetWatchingPresence(args);
    } else if (activityType == "meta-detail" && args.size() >= 4) {
        SetViewingPresence(args);
    } else if (activityType == "board") {
        SetBrowsingPresence("Resuming Favorites", "On Board", "icon-board");
    } else if (activityType == "discover") {
        SetBrowsingPresence("Finding New Gems", "In Discover", "icon-discover");
    } else if (activityType == "library") {
        SetBrowsingPresence("Revisiting Old Favorites", "In Library", "icon-library");
    } else if (activityType == "calendar") {
        SetBrowsingPresence("Planning My Next Binge", "On Calendar", "icon-calendar");
    } else if (activityType == "addons") {
        SetBrowsingPresence("Exploring Add-ons", "In Add-ons", "icon-addons");
    } else if (activityType == "settings") {
        SetBrowsingPresence("Tuning Preferences", "In Settings", "icon-settings");
    } else if (activityType == "search") {
        SetBrowsingPresence("Searching for Shows & Movies", "In Search", "icon-search");
    } else if (activityType == "clear") {
        discord::RPCManager::get().clearPresence();
    } else {
        LOG_WARN("DiscordManager", "Unknown activity type for presence: " + activityType);
    }
}

void DiscordManager::SetWatchingPresence(const std::vector<std::string>& args) {
    auto& presence = discord::RPCManager::get().getPresence();
    presence.clear();
    presence.setActivityType(discord::ActivityType::Watching);
    
    presence.setDetails("Watching " + args[2]);
    presence.setLargeImageKey(args[7]);
    presence.setLargeImageText(args[2]);

    bool isPaused = (args.size() > 10 && args[10] == "yes");

    if (isPaused) {
        presence.setState("Paused");
    } else {
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        int elapsed = std::stoi(args[8]);
        int duration = std::stoi(args[9]);
        presence.setStartTimestamp(now - elapsed);
        presence.setEndTimestamp(now + (duration - elapsed));
        
        if (args[1] == "series") {
            presence.setState(args[5] + " (S" + args[3] + "-E" + args[4] + ")");
            if (args.size() > 6 && !args[6].empty()) {
                presence.setSmallImageKey(args[6]);
                presence.setSmallImageText(args[5]);
            }
        } else {
            presence.setState("Playing");
        }
    }

    if (args.size() > 11 && !args[11].empty()) presence.setButton1("More Details", args[11]);
    if (args.size() > 12 && !args[12].empty()) presence.setButton2("Watch on Stremato", args[12]);

    presence.refresh();
}

void DiscordManager::SetViewingPresence(const std::vector<std::string>& args) {
    auto& presence = discord::RPCManager::get().getPresence();
    presence.clear();
    presence.setActivityType(discord::ActivityType::Watching);
    
    presence.setDetails("Viewing " + args[2]);
    presence.setState("Browsing Details");
    presence.setLargeImageKey(args[3]);
    presence.setLargeImageText(args[2]);
    
    presence.refresh();
}

void DiscordManager::SetBrowsingPresence(const std::string& details, const std::string& state, const std::string& iconKey) {
    auto& presence = discord::RPCManager::get().getPresence();
    presence.clear();
    presence.setActivityType(discord::ActivityType::Watching);
    presence.setDetails(details);
    presence.setState(state);
    presence.setLargeImageKey(iconKey);
    presence.setLargeImageText(state);
    presence.refresh();
}