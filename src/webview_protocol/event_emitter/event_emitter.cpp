#include "event_emitter.h"
#include "../../helpers/helpers.h"
#include "../transport_constants.h"
#include <wil/com.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;


namespace WebViewProtocol {
    namespace EventEmitter {
        static wil::com_ptr<ICoreWebView2> g_webview;
        static void emitEvent(const std::string& eventName, const json& payload) {
            if (!g_webview) return;
            json eventMessage = {
                {"event", eventName},
                {"payload", payload}
            };
            g_webview->PostWebMessageAsJson(Utf8ToWstring(eventMessage.dump()).c_str());
        }

        void SetWebViewInstance(ICoreWebView2* webview) {
            g_webview = webview;
        }

        void emitPropertyChange(const std::string &property, const json &value) {
            PropertyChangeEventPayload payload = {property, value};
            emitEvent(Events::PROPERTY_CHANGE, json(payload));
        }

        void emitPlaybackEnded() {
            emitEvent(Events::PLAYBACK_ENDED, {});
        }

        void emitPlaybackError(const std::string &message) {
            PlaybackErrorEventPayload payload = {message};
            emitEvent(Events::PLAYBACK_ERROR, json(payload));
        }
        
        void emitAuthResult(const AuthResult& result) {
            emitEvent(Events::AUTH_RESULT, json(result));
        }
        
        void emitUpdateAvailable() {
            emitEvent(Events::UPDATE_AVAILABLE, {});
        }

        void emitCommandResponse(const std::string& messageId, const std::optional<json>& result, const std::optional<std::string>& error) {
            json payload = {{"messageId", messageId}};
            if (result.has_value()) {
                payload["result"] = result.value();
            }
            if (error.has_value()) {
                payload["error"] = error.value();
            }
            emitEvent(Events::COMMAND_RESPONSE, payload);
        }

    } // namespace EventEmitter
} // namespace WebViewProtocol