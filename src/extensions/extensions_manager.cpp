#include "extensions_manager.h"
#include "../app/app_manager.h"
#include "../logger/logger.h"
#include "../helpers/helpers.h"
#include "../webview/webview_manager.h"
#include "../globals/globals.h"
#include "nlohmann/json.hpp"
#include <curl/curl.h>
#include <thread>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

ExtensionsManager::ExtensionsManager(AppManager* appManager) : m_appManager(appManager) {}

void ExtensionsManager::AddLoadedExtension(const std::wstring& name, const std::wstring& id) {
    m_extensionMap[name] = id;
}

bool ExtensionsManager::HandleNavigation(const std::wstring& uri) {
    if (HandlePremidLogin(uri)) return true;
    if (HandleStylusUsoInstall(uri)) return true;
    return false;
}

bool ExtensionsManager::HandlePremidLogin(const std::wstring& uri) {
    if (uri.rfind(L"https://login.premid.app", 0) != 0 || uri.rfind(L"https://discord.com", 0) == 0) {
        return false;
    }

    std::wstring extensionId;
    auto it = std::find_if(m_extensionMap.begin(), m_extensionMap.end(),
        [](const auto& p) { return p.first.find(L"premid") != std::wstring::npos; });

    auto webviewManager = m_appManager->GetWebViewManager();
    if (it == m_extensionMap.end()) {
        LOG_WARN("HandlePremidLogin", "PreMiD extension ID not found");
        webviewManager->Navigate(g_webuiUrl);
        return true;
    }
    extensionId = it->second;

    std::wstring codeParam;
    size_t codePos = uri.find(L"code=");
    if (codePos != std::wstring::npos) {
        codePos += 5;
        size_t ampPos = uri.find(L'&', codePos);
        codeParam = (ampPos == std::wstring::npos) ? uri.substr(codePos) : uri.substr(codePos, ampPos - codePos);
    }

    m_scriptQueue.push_back(L"globalThis.getAuthorizationCode(\"" + codeParam + L"\");");
    webviewManager->Navigate(L"chrome-extension://" + extensionId + L"/popup.html");
    return true;
}

bool ExtensionsManager::HandleStylusUsoInstall(const std::wstring& uri) {
    if (uri.rfind(L"https://raw.githubusercontent.com/uso-archive", 0) != 0) {
        return false;
    }

    std::wstring extensionId;
    auto it = std::find_if(m_extensionMap.begin(), m_extensionMap.end(),
        [](const auto& p) { return p.first.find(L"stylus") != std::wstring::npos; });

    auto webviewManager = m_appManager->GetWebViewManager();
    if (it == m_extensionMap.end()) {
        LOG_WARN("HandleStylusUsoInstall", "Stylus extension ID not found");
        // FIX: Use the globally available g_webuiUrl
        webviewManager->Navigate(g_webuiUrl);
        return true;
    }
    extensionId = it->second;
    
    webviewManager->Navigate(L"chrome-extension://" + extensionId + L"/install-usercss.html?updateUrl=" + uri);
    return true;
}

void ExtensionsManager::FetchAndParseWhitelist() {
    std::thread([this]() {
        LOG_INFO("ExtensionsManager", "Fetching extension domain whitelist...");
        std::string responseData;
        if (DownloadWhitelist(responseData)) {
            try {
                json j = json::parse(responseData);
                if (j.contains("domains") && j["domains"].is_array()) {
                    m_domainWhitelist.clear();
                    for (const auto& domain : j["domains"]) {
                        if (domain.is_string()) {
                            m_domainWhitelist.push_back(Utf8ToWstring(domain.get<std::string>()));
                        }
                    }
                    LOG_INFO("ExtensionsManager", "Successfully parsed " + std::to_string(m_domainWhitelist.size()) + " domains for whitelist.");
                }
            } catch (const json::exception& e) {
                LOG_WARN("ExtensionsManager", "Failed to parse domain whitelist JSON: " + std::string(e.what()));
            }
        } else {
            LOG_WARN("ExtensionsManager", "Failed to download domain whitelist.");
        }
    }).detach();
}

bool ExtensionsManager::DownloadWhitelist(std::string& outData) const {
    const std::string url = "https://raw.githubusercontent.com/Ali-Kabbadj/Stremato/refs/heads/Release/webview2/resources/extensions.json";
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outData);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Stremato-ExtensionsManager/1.0");
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}