#include "updater_manager.h"
#include "../app/app_manager.h"
#include "../logger/logger.h"
#include "../helpers/helpers.h"
#include "../window/window_manager.h"
#include "../server/server_manager.h"
#include "../crashlog/crashlog.h"
#include "../globals/globals.h"
#include "../webview_protocol/event_emitter/event_emitter.h"
#include "nlohmann/json.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

static const char public_key_pem[] = "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoXoJRQ81xOT3Gx6+hsWMZiD4PwtLdxxNhEdL/iK0yp6AdO/L0kcSHk9YCPPx0XPK9sssjSV5vCbNE/2IJxnh/mV+3GAMmXgMvTL+DZgrHafnxe1K50M+8Z2z+uM5YC9XDLppgnC6OrUjwRqNHrKIT1vcgKf16e/TdKj8xlgadoHBECjv6dr87nbHW115bw8PVn2tSk/zC+QdUud+p6KVzA6+FT9ZpHJvdS3R0V0l7snr2cwapXF6J36aLGjJ7UviRFVWEEsQaKtAAtTTBzdD4B9FJ2IJb/ifdnVzeuNTDYApCSE1F89XFWN9FoDyw7Jkk+7u4rsKjpcnCDTd9ziGkwIDAQAB\n-----END PUBLIC KEY-----\n"; 

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

UpdaterManager::UpdaterManager(AppManager* appManager) 
    : m_appManager(appManager), 
      m_updateUrl("https://raw.githubusercontent.com/Ali-Kabbadj/Stremato/refs/heads/webview-windows/versioning/version.json"),
      m_forceFullUpdate(false) 
{}

UpdaterManager::~UpdaterManager() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void UpdaterManager::CheckForUpdates() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_thread = std::thread(&UpdaterManager::UpdaterThread, this);
}

void UpdaterManager::RunInstallerAndExit() {
    if (m_installerPath.empty()) {
        LOG_ERROR("UpdaterManager", "Installer path not set.");
        AppendToCrashLog("Attempted to run installer, but path was empty.");
        return;
    }
    ShellExecuteW(nullptr, L"open", m_installerPath.c_str(), L"/S", nullptr, SW_SHOWNORMAL);
    PostMessage(m_appManager->GetWindowManager()->GetHWND(), WM_CLOSE, 0, 0);
}

void UpdaterManager::UpdaterThread()
{
    LOG_INFO("UpdaterManager", "Checking for Updates.");

    std::string versionContent;
    if(!DownloadString(m_updateUrl, versionContent)) {
        AppendToCrashLog("[UPDATER]: Failed to download version.json");
        return;
    }

    json versionJson;
    try {
        versionJson = json::parse(versionContent);
    } catch(...) { AppendToCrashLog("[UPDATER]: Failed to parse version.json"); return; }

    std::string versionDescUrl = versionJson["versionDesc"].get<std::string>();
    std::string signatureBase64 = versionJson["signature"].get<std::string>();

    std::string detailsContent;
    if(!DownloadString(versionDescUrl, detailsContent)) {
        AppendToCrashLog("[UPDATER]: Failed to download version details");
        return;
    }
    if(!VerifySignature(detailsContent, signatureBase64)) {
        AppendToCrashLog("[UPDATER]: Signature verification failed");
        return;
    }
    
    json detailsJson;
    try {
        detailsJson = json::parse(detailsContent);
    } catch(...) { AppendToCrashLog("[UPDATER]: Failed to parse version details JSON"); return; }

    std::string remoteShellVersion = detailsJson["shellVersion"].get<std::string>();
    bool needsFullUpdate = (remoteShellVersion != APP_VERSION);

    auto files = detailsJson["files"];
    std::vector<std::string> partialUpdateKeys = { "server.js" };

    wchar_t buf[MAX_PATH];
    GetTempPathW(MAX_PATH, buf);
    std::filesystem::path tempDir = std::filesystem::path(buf) / L"Stremato_updater";
    std::filesystem::create_directories(tempDir);

    if(needsFullUpdate || m_forceFullUpdate) {
        bool downloadOk = false;
        std::string key = "windows-x64";
        
        if(files.contains(key) && files[key].contains("url") && files[key].contains("checksum")) {
            std::string url = files[key]["url"].get<std::string>();
            std::string expectedChecksum = files[key]["checksum"].get<std::string>();
            std::string filename = url.substr(url.find_last_of('/') + 1);
            std::filesystem::path installerPath = tempDir / std::wstring(filename.begin(), filename.end());

            if (!std::filesystem::exists(installerPath) || FileChecksum(installerPath) != expectedChecksum) {
                LOG_INFO("UpdaterManager", "Downloading new installer: " + filename);
                if (DownloadFile(url, installerPath) && FileChecksum(installerPath) == expectedChecksum) {
                    downloadOk = true;
                } else {
                    AppendToCrashLog("[UPDATER]: Installer download or checksum verification failed.");
                }
            } else {
                downloadOk = true;
            }
            
            if (downloadOk) {
                m_installerPath = installerPath;
            }
        }

        if(downloadOk) {
            LOG_INFO("UpdaterManager", "Full update is ready. Notifying frontend.");
            WebViewProtocol::EventEmitter::emitUpdateAvailable();
        } else {
            LOG_WARN("UpdaterManager", "Installer download failed. Skipping update prompt.");
        }
    }

    if(!needsFullUpdate) {
        std::filesystem::path exeDirPath = GetExeDirectory();
        for(const auto& key : partialUpdateKeys) {
            if(files.contains(key) && files[key].contains("url") && files[key].contains("checksum")) {
                std::string url = files[key]["url"].get<std::string>();
                std::string expectedChecksum = files[key]["checksum"].get<std::string>();
                // FIX: Use std::filesystem::path for joining
                std::filesystem::path localFilePath = exeDirPath / std::wstring(key.begin(), key.end());

                if(std::filesystem::exists(localFilePath) && FileChecksum(localFilePath) == expectedChecksum) {
                    continue;
                }
                
                if(DownloadFile(url, localFilePath) && FileChecksum(localFilePath) == expectedChecksum) {
                    LOG_INFO("UpdaterManager", "Partial update applied for: " + key);
                    if(key=="server.js") {
                        m_appManager->GetServerManager()->Stop();
                        m_appManager->GetServerManager()->Start();
                    }
                } else {
                    AppendToCrashLog("[UPDATER]: Partial update failed for " + key);
                }
            }
        }
    }
    LOG_INFO("UpdaterManager", "Update check finished.");
}

bool UpdaterManager::DownloadString(const std::string& url, std::string& outData) const {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outData);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Stremato-Updater/1.0");
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

bool UpdaterManager::DownloadFile(const std::string& url, const std::filesystem::path& dest) const { 
    CURL* curl = curl_easy_init();
    if(!curl) return false;
    FILE* fp = _wfopen(dest.c_str(), L"wb");
    if(!fp){ curl_easy_cleanup(curl); return false; }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK);
}

std::string UpdaterManager::FileChecksum(const std::filesystem::path& filepath) const { 
    std::ifstream file(filepath, std::ios::binary);
    if(!file) return "";
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char buffer[4096];
    while(file.read(buffer, sizeof(buffer))) { SHA256_Update(&sha256, buffer, file.gcount()); }
    SHA256_Update(&sha256, buffer, file.gcount());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) { ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i]; }
    return ss.str();
}

bool UpdaterManager::VerifySignature(const std::string& data, const std::string& signatureBase64) const { 
    BIO* bio = BIO_new_mem_buf(public_key_pem, -1);
    EVP_PKEY* pubKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if(!pubKey) return false;
    std::string cleanedSig;
    for(char c : signatureBase64) if(!isspace((unsigned char)c)) cleanedSig.push_back(c);
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bmem = BIO_new_mem_buf(cleanedSig.data(), (int)cleanedSig.size());
    bmem = BIO_push(b64, bmem);
    std::vector<unsigned char> signature(EVP_PKEY_size(pubKey));
    int sig_len = BIO_read(bmem, signature.data(), (int)signature.size());
    BIO_free_all(bmem);
    if(sig_len <= 0) { EVP_PKEY_free(pubKey); return false; }
    signature.resize(sig_len);
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    bool result = false;
    if(EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pubKey) == 1){
        if(EVP_DigestVerifyUpdate(ctx, data.data(), data.size()) == 1){
            result = (EVP_DigestVerifyFinal(ctx, signature.data(), signature.size()) == 1);
        }
    }
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pubKey);
    return result;
}