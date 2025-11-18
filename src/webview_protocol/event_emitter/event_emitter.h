#ifndef EVENT_EMITTER_H
#define EVENT_EMITTER_H

#include <string>
#include <WebView2.h>
#include <optional>
#include "../types.h"

namespace WebViewProtocol {
    namespace EventEmitter {
        void SetWebViewInstance(ICoreWebView2* webview); 

        void emitPropertyChange(const std::string &property, const json &value);
        void emitPlaybackEnded();
        void emitPlaybackError(const std::string &message);
        void emitAuthResult(const AuthResult& result);
        void emitUpdateAvailable();
        void emitCommandResponse(const std::string& messageId, const std::optional<json>& result, const std::optional<std::string>& error);
    }
}

#endif // EVENT_EMITTER_H