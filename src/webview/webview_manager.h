#ifndef WEBVIEW_MANAGER_H
#define WEBVIEW_MANAGER_H

#include <windows.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>

class AppManager;

class WebViewManager
{
public:
    explicit WebViewManager(AppManager* appManager);
    ~WebViewManager() = default;

    bool Initialize(HWND parentHWnd);
    void StartInitialNavigation();
    void Resize();
    void Refresh(bool hardRefresh);
    void Navigate(const std::wstring& url);
    ICoreWebView2_21* GetWebView() const { return m_webview21.get(); }

private:
    HRESULT OnEnvironmentCreated(HRESULT result, ICoreWebView2Environment* env);
    HRESULT OnControllerCreated(HRESULT result, ICoreWebView2Controller* controller);

    void RegisterEventHandlers();

    AppManager* m_appManager;
    HWND m_parentHWnd;

    wil::com_ptr<ICoreWebView2Environment> m_webviewEnv;
    wil::com_ptr<ICoreWebView2Controller> m_webviewController;
    
    wil::com_ptr<ICoreWebView2_21> m_webview21;
    wil::com_ptr<ICoreWebView2Profile8> m_webviewProfile;
};

#endif // WEBVIEW_MANAGER_H