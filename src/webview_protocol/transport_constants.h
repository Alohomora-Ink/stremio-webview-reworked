#ifndef WEBVIEW_PROTOCOL_CONSTANTS_H
#define WEBVIEW_PROTOCOL_CONSTANTS_H

namespace WebViewProtocol {
    namespace Commands {
        constexpr const char* PLAY = "play";
        constexpr const char* STOP = "stop";
        constexpr const char* TOGGLE_PAUSE = "toggle-pause";
        constexpr const char* SEEK = "seek";
        constexpr const char* SET_VOLUME = "set-volume";
        constexpr const char* TOGGLE_MUTE = "toggle-mute";
        constexpr const char* SET_PROPERTY = "set-property";
        constexpr const char* LOAD_SUBTITLE = "load-subtitle";
        constexpr const char* TOGGLE_FULLSCREEN = "toggle-fullscreen";
        constexpr const char* FRONTEND_READY = "frontend-ready";
        constexpr const char* SET_RPC = "set-rpc";
        constexpr const char* NAVIGATE = "navigate";
        constexpr const char* UPDATE_INSTALL = "update-install";
        constexpr const char* GET_SETTING = "get-setting";
    }

    namespace Events {
        constexpr const char* COMMAND_RESPONSE = "command-response";
        constexpr const char* PROPERTY_CHANGE = "property-change";
        constexpr const char* PLAYBACK_ENDED = "playback-ended";
        constexpr const char* PLAYBACK_ERROR = "playback-error";
        constexpr const char* AUTH_RESULT = "auth-result";
        constexpr const char* UPDATE_AVAILABLE = "update-available";
    }
}

#endif // WEBVIEW_PROTOCOL_CONSTANTS_H