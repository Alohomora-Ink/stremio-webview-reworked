#include "webview_manager.h"
#include "../app/app_manager.h"
#include "../logger/logger.h"
#include "../helpers/helpers.h"
#include "../window/window_manager.h"
#include "../webview_protocol/event_emitter/event_emitter.h"
#include "../extensions/extensions_manager.h"

#include <thread>
#include <sstream>
#include <objbase.h>

static const wchar_t* COMMUNICATION_BRIDGE_SCRIPT = LR"JS(
(function(){
    if (window.self === window.top && !window.qt) {
      window.qt = { webChannelTransport: { send: window.chrome.webview.postMessage, onmessage: (ev) => {} } };
      window.chrome.webview.addEventListener('message', (ev) => { window.qt.webChannelTransport.onmessage(ev); });
    }
})();
)JS";

WebViewManager::WebViewManager(AppManager* appManager)
    : m_appManager(appManager), m_parentHWnd(nullptr) {}

bool WebViewManager::Initialize(HWND parentHWnd) {
    m_parentHWnd = parentHWnd;
    LOG_INFO("WebViewManager", "Starting WebView2 initialization...");

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required --disable-features=msWebOOUI,msPdfOOUI,msSmartScreenProtection");
    std::wstring userDataFolder = GetExeDirectory() + L"\\portable_config\\WebView2_Data";

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataFolder.c_str(), options.Get(),
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(this, &WebViewManager::OnEnvironmentCreated).Get());

    if (FAILED(hr)) {
        LOG_ERROR("WebViewManager", "Top-level CreateCoreWebView2EnvironmentWithOptions call failed. HRESULT: " + std::to_string(hr));
        return false;
    }
    return true;
}

HRESULT WebViewManager::OnEnvironmentCreated(HRESULT result, ICoreWebView2Environment* env) {
    if (FAILED(result) || !env) {
        LOG_ERROR("WebViewManager", "Failed to create WebView2 Environment. HRESULT: " + std::to_string(result));
        return E_FAIL;
    }
    m_webviewEnv = env;
    return m_webviewEnv->CreateCoreWebView2Controller(m_parentHWnd,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(this, &WebViewManager::OnControllerCreated).Get());
}

HRESULT WebViewManager::OnControllerCreated(HRESULT result, ICoreWebView2Controller* controller) {
    if (FAILED(result) || !controller) {
        LOG_ERROR("WebViewManager", "Failed to create WebView2 Controller. HRESULT: " + std::to_string(result));
        return E_FAIL;
    }

    m_webviewController = controller;
    m_webviewController->put_IsVisible(TRUE);

    wil::com_ptr<ICoreWebView2> coreWebView;
    m_webviewController->get_CoreWebView2(&coreWebView);
    if (!coreWebView) {
        LOG_ERROR("WebViewManager", "get_CoreWebView2 failed.");
        return E_FAIL;
    }

    m_webview21 = coreWebView.try_query<ICoreWebView2_21>();
    if (!m_webview21) {
        LOG_ERROR("WebViewManager", "FATAL: try_query for ICoreWebView2_21 returned NULL. Your system's WebView2 Runtime is OUTDATED. Please update it.");
        MessageBoxW(m_parentHWnd, L"Your system's WebView2 Runtime is outdated. Please update it from Microsoft's website to run this application.", L"WebView2 Error", MB_OK | MB_ICONERROR);
        return E_FAIL;
    }
    
    wil::com_ptr<ICoreWebView2Profile> profile;
    m_webview21->get_Profile(&profile);
    if (profile) {
         m_webviewProfile = profile.try_query<ICoreWebView2Profile8>();
         if (!m_webviewProfile) {
             LOG_WARN("WebViewManager", "Failed to get ICoreWebView2Profile8. Extensions may not load.");
         }
    } else {
        LOG_WARN("WebViewManager", "get_Profile returned null.");
    }

    wil::com_ptr<ICoreWebView2Settings> settings;
    m_webview21->get_Settings(&settings);
    if (settings) {
        settings->put_AreDevToolsEnabled(TRUE);
        settings->put_IsStatusBarEnabled(FALSE);
    }
    
    auto controller2 = m_webviewController.try_query<ICoreWebView2Controller2>();
    if (controller2) {
        COREWEBVIEW2_COLOR color = {0, 0, 0, 0};
        controller2->put_DefaultBackgroundColor(color);
    }

    Resize();
    m_webview21->AddScriptToExecuteOnDocumentCreated(COMMUNICATION_BRIDGE_SCRIPT, nullptr);
    WebViewProtocol::EventEmitter::SetWebViewInstance(m_webview21.get());
    RegisterEventHandlers();
    m_appManager->GetExtensionsManager()->FetchAndParseWhitelist();

    StartInitialNavigation();

    LOG_INFO("WebViewManager", "WebView2 initialization sequence complete.");
    return S_OK;
}

void WebViewManager::StartInitialNavigation() {
    std::thread([this]() {
        g_webuiUrl = GetFirstReachableUrl();
        LOG_INFO("WebViewManager", "Found reachable UI: " + WStringToUtf8(g_webuiUrl));
        auto* urlToNavigate = new std::wstring(g_webuiUrl);
        PostMessage(m_parentHWnd, WM_NAVIGATE_READY, 0, (LPARAM)urlToNavigate);
    }).detach();
}

void WebViewManager::Navigate(const std::wstring& url) {
    if (m_webview21) {
        m_webview21->Navigate(url.c_str());
    }
}

void WebViewManager::RegisterEventHandlers() {
    if (!m_webview21) return;

    m_webview21->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                wil::unique_cotaskmem_string msg;
                args->TryGetWebMessageAsString(&msg);
                if (msg) m_appManager->GetCommandHandler()->HandleCommand(msg.get());
                return S_OK;
            }).Get(), nullptr);

    m_webview21->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                BOOL isSuccess;
                args->get_IsSuccess(&isSuccess);
                if (!isSuccess) {
                    COREWEBVIEW2_WEB_ERROR_STATUS status;
                    args->get_WebErrorStatus(&status);
                    LOG_WARN("WebViewManager", "Navigation failed with status: " + std::to_string(status));
                } else {
                    LOG_INFO("WebViewManager", "Navigation successful.");
                }
                return S_OK;
            }).Get(), nullptr);

    m_webview21->add_NewWindowRequested(
        Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                wil::unique_cotaskmem_string uri;
                args->get_Uri(&uri);
                if (uri) {
                    ShellExecuteW(nullptr, L"open", uri.get(), nullptr, nullptr, SW_SHOWNORMAL);
                }
                args->put_Handled(TRUE);
                return S_OK;
            }).Get(), nullptr);

    m_webview21->add_ContainsFullScreenElementChanged(
        Microsoft::WRL::Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>(
            [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                BOOL isFullscreen;
                sender->get_ContainsFullScreenElement(&isFullscreen);
                if ((bool)isFullscreen != m_appManager->GetWindowManager()->IsFullScreen()) {
                    m_appManager->GetWindowManager()->ToggleFullScreen();
                }
                return S_OK;
            }).Get(), nullptr);
}

void WebViewManager::Resize() {
    if (m_webviewController) {
        RECT bounds;
        GetClientRect(m_parentHWnd, &bounds);
        m_webviewController->put_Bounds(bounds);
    }
}

void WebViewManager::Refresh(bool hardRefresh) {
    if (m_webview21) {
        m_webview21->Reload();
    }
}