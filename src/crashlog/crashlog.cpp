#include "crashlog.h"
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include "../helpers/helpers.h"

static std::wstring GetDailyCrashLogPath()
{
    std::time_t t = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &t);

    std::wstringstream filename;
    filename << L"\\errors-"
             << localTime.tm_mday << L"."
             << (localTime.tm_mon + 1) << L"."
             << (localTime.tm_year + 1900) << L".txt";

    std::wstring exeDir = GetExeDirectory();
    std::wstring pcDir  = exeDir + L"\\portable_config";
    CreateDirectoryW(pcDir.c_str(), nullptr);
    return pcDir + filename.str();
}

void AppendToCrashLog(const std::wstring& message)
{
    std::wofstream logFile;
    logFile.open(GetDailyCrashLogPath(), std::ios::app);
    if(!logFile.is_open()) {
        return;
    }
    std::time_t t = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &t);
    logFile << L"[" << std::put_time(&localTime, L"%H:%M:%S") << L"] "
            << message << std::endl;
}

void AppendToCrashLog(const std::string& message)
{
    AppendToCrashLog(Utf8ToWstring(message));
}