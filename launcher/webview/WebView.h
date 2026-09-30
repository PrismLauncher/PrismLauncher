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

#pragma once

#include <QByteArray>
#include <QSize>
#include <QString>
#include <QUrl>

#include <functional>
#include <memory>

namespace webview {

/** Scheme + host under which the bundled UI is served: `materialmc://app/index.html`. */
inline constexpr auto AppScheme = "materialmc";
inline constexpr auto AppHost = "app";

/** Response produced by the host for a request to `materialmc://app/<path>`. */
struct Response {
    int status = 200;
    QByteArray mimeType;
    QByteArray body;

    static Response notFound() { return { 404, "text/plain", "Not found" }; }
    static Response forbidden() { return { 403, "text/plain", "Forbidden" }; }
};

struct Options {
    QString title;
    QSize size{ 1200, 780 };
    QSize minimumSize{ 720, 480 };
    /** Enables the inspector and the context menu. */
    bool developerTools = false;
    /** Origin (scheme://host:port) that may use the bridge besides the app scheme, e.g. the Vite dev server. */
    QString devOrigin;
    /** Freedesktop icon name / resource used for the window icon (platform specific, may be ignored). */
    QString iconName;
};

/**
 * A native top-level window hosting the platform's system WebView.
 *
 * Implementations:
 *  - Linux/BSD: WebKitGTK 4.1  (LinuxWebView.cpp)
 *  - Windows:   WebView2       (WindowsWebView.cpp, TODO)
 *  - macOS:     WKWebView      (MacOSWebView.mm, TODO)
 *
 * The contract every implementation must honour (see docs/react-webview.md, "Security model"):
 *  1. Inject bootstrapScript() before any page script, exposing only `window.__materialmcNative.post(string)`.
 *  2. Deliver host->page messages exclusively through deliverScript() (a call of the fixed
 *     `window.__materialmcReceive` function with a JSON string literal); never evaluate other scripts.
 *  3. Accept page->host messages only from the app origin (or `Options::devOrigin` when set).
 *  4. Serve `materialmc://app/...` through the ResourceHandler; add the Content-Security-Policy from
 *     contentSecurityPolicy() to every response.
 *  5. Refuse navigation away from the app origin; hand http(s) links to the ExternalUrlHandler instead.
 *  6. Never open popups / new windows.
 *
 * All methods must be called on the GUI thread; all callbacks are invoked on the GUI thread.
 */
class WebView {
   public:
    using MessageHandler = std::function<void(const QByteArray& message)>;
    using ResourceHandler = std::function<Response(const QString& path)>;
    using ExternalUrlHandler = std::function<void(const QUrl& url)>;
    /** Return true to let the window close, false to keep it open. */
    using CloseHandler = std::function<bool()>;

    virtual ~WebView() = default;

    /**
     * Creates the WebView for the current platform.
     * Returns nullptr and fills `error` when no implementation is available or initialisation failed.
     */
    static std::unique_ptr<WebView> create(const Options& options, QString* error);

    /** Human readable engine name and version, e.g. "WebKitGTK 2.52.6". */
    virtual QString engineName() const = 0;

    virtual void load(const QUrl& url) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    /** Raises and focuses the window. */
    virtual void present() = 0;
    virtual bool isVisible() const = 0;
    virtual void setTitle(const QString& title) = 0;

    /** Sends a JSON message to the page (`window.__materialmcReceive`). */
    virtual void postMessage(const QByteArray& json) = 0;

    void setMessageHandler(MessageHandler handler) { m_messageHandler = std::move(handler); }
    void setResourceHandler(ResourceHandler handler) { m_resourceHandler = std::move(handler); }
    void setExternalUrlHandler(ExternalUrlHandler handler) { m_externalUrlHandler = std::move(handler); }
    void setCloseHandler(CloseHandler handler) { m_closeHandler = std::move(handler); }

    /** The URL of the bundled UI entry point. */
    static QUrl appUrl(const QString& path = QStringLiteral("index.html"));

    /** True for URLs the bridge trusts: the app scheme/host, or `devOrigin`. */
    static bool isTrustedUrl(const QUrl& url, const QString& devOrigin);

   protected:
    /** JavaScript injected at document start; defines `window.__materialmcNative`. `postExpression` receives `s`. */
    static QByteArray bootstrapScript(const QByteArray& postExpression);
    /** Script that delivers `json` to the page. The payload is embedded as an escaped string literal. */
    static QByteArray deliverScript(const QByteArray& json);
    static QByteArray contentSecurityPolicy();
    /** Guesses a MIME type (without parameters: WebKitGTK does not parse them) from a file name. */
    static QByteArray mimeTypeFor(const QString& path);

    MessageHandler m_messageHandler;
    ResourceHandler m_resourceHandler;
    ExternalUrlHandler m_externalUrlHandler;
    CloseHandler m_closeHandler;
};

}  // namespace webview
