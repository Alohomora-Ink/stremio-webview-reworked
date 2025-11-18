#include "../helpers/helpers.h"
#include "../globals/globals.h" 
#include "logger/logger.h"
#include <fstream>
#include <shellscalingapi.h>
#include <tlhelp32.h>
#include <VersionHelpers.h>
#include <curl/curl.h>
#include <sstream>
#include <algorithm>

std::string WStringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty()) return {};
    int neededSize = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (neededSize <= 0) return {};
    std::string result(neededSize, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], neededSize, nullptr, nullptr);
    while(!result.empty() && result.back()=='\0') result.pop_back();
    return result;
}

std::wstring Utf8ToWstring(const std::string& utf8Str)
{
    if (utf8Str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), NULL, 0);
    if (size_needed == 0) return std::wstring();
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), &wstr[0], size_needed);
    return wstr;
}

bool FileExists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring GetExeDirectory()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr,buf,MAX_PATH);
    std::wstring path(buf);
    size_t pos=path.find_last_of(L"\\/");
    if(pos!=std::wstring::npos) path.erase(pos);
    return path;
}

bool isSubtitle(const std::wstring& filePath) {
    std::wstring lowerFilePath = filePath;
    std::transform(lowerFilePath.begin(), lowerFilePath.end(), lowerFilePath.begin(), towlower);
    return std::any_of(g_subtitleExtensions.begin(), g_subtitleExtensions.end(),
        [&](const std::wstring& ext) {
            if (lowerFilePath.length() >= ext.length()) {
                // Use rfind for a simpler ends_with check
                return lowerFilePath.rfind(ext) == (lowerFilePath.length() - ext.length());
            }
            return false;
        });
}

bool IsEndpointReachable(const std::wstring& url)
{
    std::string urlUtf8 = WStringToUtf8(url);
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, urlUtf8.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

std::wstring GetFirstReachableUrl() {
    for (const auto& url : g_webuiUrls) {
        if (IsEndpointReachable(url)) {
            return url;
        }
    }
    return g_webuiUrls.empty() ? L"" : g_webuiUrls[0];
}

bool ReadFileUtf8(const std::wstring& path, std::string& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (size > 0) f.read(&out[0], size);
    return true;
}

std::string Base64Encode(const std::string& in)
{
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(T[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(T[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::wstring MakeInjectCssScript(const std::wstring& idSafe, const std::string& cssUtf8)
{
    const std::string b64 = Base64Encode(cssUtf8);
    const std::wstring wb64 = Utf8ToWstring(b64);
    std::wstringstream ss;
    ss << L"(function(){try{var id='webmods-css-" << idSafe << L"';function inject(){try{var root=document.head||document.documentElement;if(!root){setTimeout(inject,25);return;}if(document.getElementById(id))return;var bin=atob('" << wb64 << L"');var css=decodeURIComponent(escape(bin));var s=document.createElement('style');s.id=id;s.textContent=css;root.appendChild(s);}catch(e){}}inject();}catch(e){}})();";
    return ss.str();
}

std::wstring MakeInjectJsScript(const std::wstring&,const std::string& jsUtf8)
{
    const std::string b64 = Base64Encode(jsUtf8);
    const std::wstring wb64 = Utf8ToWstring(b64);
    std::wstringstream ss;
    ss << L"(function(){try{var bin=atob('" << wb64 << L"');var js=decodeURIComponent(escape(bin));(0,eval)(js);}catch(e){}})();";
    return ss.str();
}