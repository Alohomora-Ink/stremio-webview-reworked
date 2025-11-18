#ifndef HELPERS_H
#define HELPERS_H

#include <windows.h>
#include <string>
#include <vector> 
#include <filesystem>
#include <algorithm>

std::string WStringToUtf8(const std::wstring &wstr);
std::wstring Utf8ToWstring(const std::string& utf8Str);
std::wstring GetExeDirectory();
std::wstring GetFirstReachableUrl();
std::wstring MakeInjectCssScript(const std::wstring& idSafe, const std::string& cssUtf8);
std::wstring MakeInjectJsScript(const std::wstring& idSafe, const std::string& jsUtf8);
bool FileExists(const std::wstring& path);
bool isSubtitle(const std::wstring& filePath);
bool IsEndpointReachable(const std::wstring& url);
bool ReadFileUtf8(const std::wstring& path, std::string& out);

#endif // HELPERS_H