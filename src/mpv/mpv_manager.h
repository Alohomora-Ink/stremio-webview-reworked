#ifndef MPV_MANAGER_H
#define MPV_MANAGER_H

#include <windows.h>
#include <string>
#include <vector>
#include <mpv/client.h>
#include "../webview_protocol/types.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class AppManager;

class MPVManager
{
public:
    explicit MPVManager(AppManager* appManager);
    ~MPVManager();

    bool Initialize(HWND videoHostWindow);
    void HandleEvents();

    void Play(const WebViewProtocol::PlayPayload& payload);
    void Stop();
    void TogglePause();
    void Pause();
    void Seek(const WebViewProtocol::SeekPayload& payload);
    void SetVolume(const WebViewProtocol::SetVolumePayload& payload);
    void ToggleMute();
    void SetProperty(const WebViewProtocol::SetPropertyPayload& payload);
    void LoadSubtitle(const WebViewProtocol::LoadSubtitlePayload& payload);

private:
    static void MpvWakeupCallback(void* ctx);
    void HandleMpvCommand(const std::vector<std::string>& args);
    static json MpvNodeToJson(const mpv_node* node);

    AppManager* m_appManager;
    mpv_handle* m_mpv;
    HWND m_hwnd;
};

#endif // MPV_MANAGER_H