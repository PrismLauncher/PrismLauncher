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

// macOS WKWebView backend for webview::WebView.
//
// TODO(webui-macos): implement with WebKit.framework:
//  - window:     NSWindow + WKWebView (Qt already runs the Cocoa run loop, no extra loop needed)
//  - bootstrap:  WKUserScript(bootstrapScript("window.webkit.messageHandlers.materialmc.postMessage(s);"),
//                WKUserScriptInjectionTimeAtDocumentStart, forMainFrameOnly: YES)
//  - UI -> host: WKScriptMessageHandler "materialmc"; accept only if message.frameInfo.isMainFrame and
//                isTrustedUrl(message.frameInfo.request.URL)
//  - host -> UI: -[WKWebView evaluateJavaScript:deliverScript(json) completionHandler:nil]
//  - resources:  WKURLSchemeHandler registered for AppScheme on WKWebViewConfiguration, serving
//                m_resourceHandler with a Content-Security-Policy header (NSHTTPURLResponse)
//  - navigation: WKNavigationDelegate decidePolicyForNavigationAction -> allow trusted only,
//                open http(s) via m_externalUrlHandler; WKUIDelegate createWebViewWithConfiguration -> nil
//  - devtools:   WKWebView.inspectable = developerTools (macOS 13.3+)
//
// Until then the factory reports "not implemented" and the launcher keeps using the Qt Widgets GUI.

#include "WebView.h"

namespace webview {

std::unique_ptr<WebView> createMacOSWebView(const Options&, QString* error)
{
    if (error) {
        *error = QStringLiteral("The WKWebView backend is not implemented yet.");
    }
    return nullptr;
}

}  // namespace webview
