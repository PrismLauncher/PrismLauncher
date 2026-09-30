// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Windows WebView2 backend for webview::WebView.
//
// TODO(webui-windows): implement with the WebView2 SDK (Microsoft.Web.WebView2, vcpkg port "webview2"):
//  - window:     a plain Win32 top-level window (or a QWindow's native HWND) hosting ICoreWebView2Controller
//  - bootstrap:  ICoreWebView2::AddScriptToExecuteOnDocumentCreated(bootstrapScript(
//                    "window.chrome.webview.postMessage(s);"))
//  - UI -> host: ICoreWebView2::add_WebMessageReceived, check args->get_Source() with isTrustedUrl(),
//                read args->TryGetWebMessageAsString()
//  - host -> UI: ICoreWebView2::ExecuteScript(deliverScript(json))
//  - resources:  AddWebResourceRequestedFilter(L"https://materialmc.app/*") and serve from
//                m_resourceHandler (WebView2 only supports custom schemes via
//                ICoreWebView2EnvironmentOptions4::SetCustomSchemeRegistrations; either is fine as long as
//                appUrl()/isTrustedUrl() are adapted consistently), adding contentSecurityPolicy()
//  - navigation: add_NavigationStarting -> cancel untrusted URIs, forward http(s) to m_externalUrlHandler;
//                add_NewWindowRequested -> put_Handled(TRUE)
//  - settings:   put_AreDevToolsEnabled / put_AreDefaultContextMenusEnabled(developerTools),
//                put_IsGeneralAutofillEnabled(FALSE), put_IsPasswordAutosaveEnabled(FALSE)
//  - lifetime:   environment creation is asynchronous; queue load()/postMessage() until the controller exists
//
// Until then the factory reports "not implemented" and the launcher keeps using the Qt Widgets GUI.

#include "WebView.h"

namespace webview {

std::unique_ptr<WebView> createWindowsWebView(const Options&, QString* error)
{
    if (error) {
        *error = QStringLiteral("The WebView2 backend is not implemented yet.");
    }
    return nullptr;
}

}  // namespace webview
