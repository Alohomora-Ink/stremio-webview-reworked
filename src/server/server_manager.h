#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include <windows.h>
#include <string>

class ServerManager
{
public:
    ServerManager();
    ~ServerManager();

    bool Start();
    void Stop();

private:
    HANDLE m_serverJob;
    HANDLE m_nodeProcess;
};

#endif // SERVER_MANAGER_H