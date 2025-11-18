#ifndef UPDATER_MANAGER_H
#define UPDATER_MANAGER_H

#include <string>
#include <filesystem>
#include <thread>
#include "nlohmann/json.hpp"

class AppManager;

class UpdaterManager
{
public:
    explicit UpdaterManager(AppManager* appManager);
    ~UpdaterManager();

    void CheckForUpdates();
    void RunInstallerAndExit();
    
    void SetUpdateUrl(const std::string& url) { m_updateUrl = url; }
    void SetForceFullUpdate(bool force) { m_forceFullUpdate = force; }

private:
    void UpdaterThread();
    bool DownloadString(const std::string& url, std::string& outData) const;
    bool DownloadFile(const std::string& url, const std::filesystem::path& dest) const;
    std::string FileChecksum(const std::filesystem::path& filepath) const;
    bool VerifySignature(const std::string& data, const std::string& signatureBase64) const;

    AppManager* m_appManager;
    std::filesystem::path m_installerPath;
    std::thread m_thread;
        std::string m_updateUrl;
    bool m_forceFullUpdate;
};

#endif // UPDATER_MANAGER_H