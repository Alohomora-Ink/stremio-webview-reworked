#include "mpv_manager.h"
#include "../app/app_manager.h"
#include "../logger/logger.h"
#include "../webview_protocol/event_emitter/event_emitter.h"
#include "../helpers/helpers.h"
#include "../globals/globals.h"

json MPVManager::MpvNodeToJson(const mpv_node* node)
{
    if(!node) return nullptr;
    switch(node->format) {
        case MPV_FORMAT_STRING: return node->u.string ? node->u.string : "";
        case MPV_FORMAT_INT64: return node->u.int64;
        case MPV_FORMAT_DOUBLE: return node->u.double_;
        case MPV_FORMAT_FLAG: return (bool)node->u.flag;
        case MPV_FORMAT_NODE_ARRAY: {
            json j = json::array();
            for(int i = 0; i < node->u.list->num; i++) j.push_back(MpvNodeToJson(&node->u.list->values[i]));
            return j;
        }
        case MPV_FORMAT_NODE_MAP: {
            json j = json::object();
            for(int i = 0; i < node->u.list->num; i++) {
                j[node->u.list->keys[i]] = MpvNodeToJson(&node->u.list->values[i]);
            }
            return j;
        }
        default: return "<unhandled_mpv_format>";
    }
}

MPVManager::MPVManager(AppManager* appManager) : m_appManager(appManager), m_mpv(nullptr), m_hwnd(nullptr) {}

MPVManager::~MPVManager()
{
    if (m_mpv)
    {
        mpv_terminate_destroy(m_mpv);
    }
}

bool MPVManager::Initialize(HWND videoHostWindow)
{
    m_hwnd = videoHostWindow;
    m_mpv = mpv_create();
    if (!m_mpv) { LOG_ERROR("MPVManager", "mpv_create failed"); return false; }
    
    const auto& settings = m_appManager->GetSettings();

    int64_t wid = (int64_t)m_hwnd;
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);
    mpv_set_option_string(m_mpv, "vo", settings.initialVO.c_str());
    mpv_set_option_string(m_mpv, "config", "yes");
    
    std::wstring cfgDir = GetExeDirectory() + L"\\portable_config";
    CreateDirectoryW(cfgDir.c_str(), nullptr);
    mpv_set_option_string(m_mpv, "config-dir", WStringToUtf8(cfgDir).c_str());
    
    mpv_set_wakeup_callback(m_mpv, MPVManager::MpvWakeupCallback, this);

    if (mpv_initialize(m_mpv) < 0) { LOG_ERROR("MPVManager", "mpv_initialize failed"); return false; }
    
    std::string initialVolumeStr = std::to_string(settings.initialVolume);
    mpv_set_property_string(m_mpv, "volume", initialVolumeStr.c_str());
    
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 0, "volume", MPV_FORMAT_NODE);
    mpv_observe_property(m_mpv, 0, "mute", MPV_FORMAT_NODE);

    LOG_INFO("MPVManager", "MPV initialized successfully.");
    return true;
}

void MPVManager::MpvWakeupCallback(void* ctx)
{
    MPVManager* self = static_cast<MPVManager*>(ctx);
    if (self && self->m_hwnd) PostMessage(self->m_hwnd, WM_MPV_WAKEUP, 0, 0);
}


void MPVManager::HandleEvents()
{
    if (!m_mpv) return;
    while (true) {
        mpv_event* ev = mpv_wait_event(m_mpv, 0);
        if (!ev || ev->event_id == MPV_EVENT_NONE) break;

        switch(ev->event_id) {
            case MPV_EVENT_PROPERTY_CHANGE: {
                mpv_event_property* prop = (mpv_event_property*)ev->data;
                if (prop && prop->name && prop->data) {
                    json value = MpvNodeToJson((mpv_node*)prop->data);
                    WebViewProtocol::EventEmitter::emitPropertyChange(prop->name, value);
                    if (strcmp(prop->name, "volume") == 0 && value.is_number()) {
                        m_appManager->GetSettings().initialVolume = value.get<int>();
                    }
                }
                break;
            }
            case MPV_EVENT_END_FILE: {
                mpv_event_end_file* ef = (mpv_event_end_file*)ev->data;
                if (ef->reason == MPV_END_FILE_REASON_ERROR) {
                    WebViewProtocol::EventEmitter::emitPlaybackError(mpv_error_string(ef->error));
                } else {
                    WebViewProtocol::EventEmitter::emitPlaybackEnded();
                }
                break;
            }
            case MPV_EVENT_SHUTDOWN: {
                LOG_INFO("MPVManager", "MPV shutdown event received.");
                m_mpv = nullptr;
                break;
            }
            default: break;
        }
    }
}
void MPVManager::Play(const WebViewProtocol::PlayPayload& p) { HandleMpvCommand({"loadfile", p.url, "replace"}); HandleMpvCommand({"set", "start", std::to_string(p.startTime)}); }
void MPVManager::Stop() { HandleMpvCommand({"stop"}); }
void MPVManager::TogglePause() { HandleMpvCommand({"cycle", "pause"}); }
void MPVManager::Pause() { HandleMpvCommand({"set", "pause", "yes"}); }
void MPVManager::Seek(const WebViewProtocol::SeekPayload& p) { HandleMpvCommand({"seek", std::to_string(p.time), "absolute"}); }
void MPVManager::SetVolume(const WebViewProtocol::SetVolumePayload& p) { HandleMpvCommand({"set", "volume", std::to_string(p.volume)}); }
void MPVManager::ToggleMute() { HandleMpvCommand({"cycle", "mute"}); }
void MPVManager::SetProperty(const WebViewProtocol::SetPropertyPayload& p) { HandleMpvCommand({"set", p.property, p.value}); }
void MPVManager::LoadSubtitle(const WebViewProtocol::LoadSubtitlePayload& p) { HandleMpvCommand({"sub-add", p.url, "select", p.url}); }

void MPVManager::HandleMpvCommand(const std::vector<std::string>& args)
{
    if (!m_mpv || args.empty()) return;
    std::vector<const char*> cargs;
    cargs.reserve(args.size() + 1);
    for (const auto& s : args) cargs.push_back(s.c_str());
    cargs.push_back(nullptr);
    mpv_command_async(m_mpv, 0, cargs.data());
}