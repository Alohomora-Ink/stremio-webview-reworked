#ifndef DISCORD_MANAGER_H
#define DISCORD_MANAGER_H

#include <string>
#include <vector>
#include <discord-rpc.hpp>

class AppManager;

class DiscordManager
{
public:
    explicit DiscordManager(AppManager* appManager);
    ~DiscordManager();

    void Initialize();
    void SetPresence(const std::vector<std::string>& args);

private:
    void SetWatchingPresence(const std::vector<std::string>& args);
    void SetViewingPresence(const std::vector<std::string>& args);
    void SetBrowsingPresence(const std::string& details, const std::string& state, const std::string& iconKey);
    
    AppManager* m_appManager; 
};

#endif // DISCORD_MANAGER_H