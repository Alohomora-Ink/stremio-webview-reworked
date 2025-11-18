#ifndef WEBVIEW_PROTOCOL_TYPES_H
#define WEBVIEW_PROTOCOL_TYPES_H

#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace WebViewProtocol
{
    //================================================================
    // INCOMING COMMANDS (From Frontend -> C++)
    //================================================================

    struct BeginGoogleAuthPayload
    {
        std::string clientId;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BeginGoogleAuthPayload, clientId)

    struct AuthResult {
        std::string code;
        std::string redirectUri;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthResult, code, redirectUri)

    struct PlayPayload
    {
        std::string url;
        double startTime = 0.0;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlayPayload, url, startTime)

    struct SeekPayload
    {
        double time;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SeekPayload, time)

    struct SetVolumePayload
    {
        int volume;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetVolumePayload, volume)

    struct SetPropertyPayload
    {
        std::string property;
        std::string value;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetPropertyPayload, property, value)

    struct LoadSubtitlePayload
    {
        std::string url;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoadSubtitlePayload, url)

    //================================================================
    // OUTGOING EVENTS (From C++ -> Frontend)
    //================================================================

    struct PropertyChangeEventPayload
    {
        std::string property;
        json value;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PropertyChangeEventPayload, property, value)

    struct PlaybackErrorEventPayload
    {
        std::string message;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PlaybackErrorEventPayload, message)

} // namespace WebViewProtocol

#endif // WEBVIEW_PROTOCOL_TYPES_H