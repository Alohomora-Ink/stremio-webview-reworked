#include "server_manager.h"
#include "../helpers/helpers.h"
#include "../logger/logger.h"
#include <shlobj.h> 

ServerManager::ServerManager() : m_serverJob(nullptr), m_nodeProcess(nullptr) {}

ServerManager::~ServerManager()
{
    Stop();
}

bool ServerManager::Start()
{
    std::wstring exeDir = GetExeDirectory();
    std::wstring exePath = exeDir + L"\\stremio-runtime.exe";
    std::wstring scriptPath = exeDir + L"\\server.js";

    if (!FileExists(exePath) || !FileExists(scriptPath)) {
        LOG_WARN("ServerManager", "stremio-runtime.exe not found in app directory, checking %localappdata%.");
        wchar_t localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
            exeDir = std::wstring(localAppData) + L"\\Programs\\StremioService";
            exePath = exeDir + L"\\stremio-runtime.exe";
            scriptPath = exeDir + L"\\server.js";
        }
    }

    if (!FileExists(exePath) || !FileExists(scriptPath)) {
        LOG_ERROR("ServerManager", "Could not find stremio-runtime.exe and server.js.");
        return false;
    }

    m_serverJob = CreateJobObject(nullptr, nullptr);
    if (m_serverJob) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {0};
        jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(m_serverJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo));
    } else {
        LOG_ERROR("ServerManager", "Failed to create Job Object.");
        return false;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {0};
    std::wstring cmdLine = L"\"" + exePath + L"\" \"" + scriptPath + L"\"";

    BOOL success = CreateProcessW(
        nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, exeDir.c_str(), &si, &pi
    );

    if (!success) {
        LOG_ERROR("ServerManager", "Failed to launch stremio-runtime.exe. Error: " + std::to_string(GetLastError()));
        return false;
    }

    AssignProcessToJobObject(m_serverJob, pi.hProcess);
    m_nodeProcess = pi.hProcess;
    CloseHandle(pi.hThread);

    LOG_INFO("ServerManager", "Node server process launched successfully.");
    return true;
}

void ServerManager::Stop()
{
    if (m_serverJob) {
        CloseHandle(m_serverJob);
        m_serverJob = nullptr;
        m_nodeProcess = nullptr; 
        LOG_INFO("ServerManager", "Node server stopped via Job Object.");
    }
}