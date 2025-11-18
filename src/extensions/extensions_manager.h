#ifndef EXTENSIONS_MANAGER_H
#define EXTENSIONS_MANAGER_H

#include <string>
#include <vector>
#include <map>

class AppManager; 

class ExtensionsManager
{
public:
    explicit ExtensionsManager(AppManager* appManager);
    ~ExtensionsManager() = default;

    void AddLoadedExtension(const std::wstring& name, const std::wstring& id);
    bool HandleNavigation(const std::wstring& uri);
    const std::vector<std::wstring>& GetScriptQueue() const { return m_scriptQueue; }
    void ClearScriptQueue() { m_scriptQueue.clear(); }
    void FetchAndParseWhitelist();

private:
    bool HandlePremidLogin(const std::wstring& uri);
    bool HandleStylusUsoInstall(const std::wstring& uri);
    bool DownloadWhitelist(std::string& outData) const;

    AppManager* m_appManager;
    std::map<std::wstring, std::wstring> m_extensionMap;
    std::vector<std::wstring> m_scriptQueue;
    std::vector<std::wstring> m_domainWhitelist;
};

#endif // EXTENSIONS_MANAGER_H