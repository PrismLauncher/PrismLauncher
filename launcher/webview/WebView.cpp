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

#include "WebView.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace webview {

#if defined(MATERIALMC_WEBVIEW_WEBKITGTK)
std::unique_ptr<WebView> createLinuxWebView(const Options& options, QString* error);
#elif defined(MATERIALMC_WEBVIEW_WEBVIEW2)
std::unique_ptr<WebView> createWindowsWebView(const Options& options, QString* error);
#elif defined(MATERIALMC_WEBVIEW_WKWEBVIEW)
std::unique_ptr<WebView> createMacOSWebView(const Options& options, QString* error);
#endif

std::unique_ptr<WebView> WebView::create(const Options& options, QString* error)
{
#if defined(MATERIALMC_WEBVIEW_WEBKITGTK)
    return createLinuxWebView(options, error);
#elif defined(MATERIALMC_WEBVIEW_WEBVIEW2)
    return createWindowsWebView(options, error);
#elif defined(MATERIALMC_WEBVIEW_WKWEBVIEW)
    return createMacOSWebView(options, error);
#else
    (void)options;
    if (error) {
        *error = QStringLiteral("No system WebView implementation is available for this platform.");
    }
    return nullptr;
#endif
}

QUrl WebView::appUrl(const QString& path)
{
    QUrl url;
    url.setScheme(AppScheme);
    url.setHost(AppHost);
    url.setPath('/' + path);
    return url;
}

bool WebView::isTrustedUrl(const QUrl& url, const QString& devOrigin)
{
    if (url.scheme() == QLatin1String(AppScheme) && url.host() == QLatin1String(AppHost)) {
        return true;
    }
    if (devOrigin.isEmpty()) {
        return false;
    }
    const QUrl origin(devOrigin);
    return url.scheme() == origin.scheme() && url.host() == origin.host() && url.port() == origin.port();
}

QByteArray WebView::bootstrapScript(const QByteArray& postExpression)
{
    // Runs before any page script. Exposes exactly one frozen, non-configurable function to the page.
    return QByteArrayLiteral(R"JS((() => {
  "use strict";
  if (window.top !== window) return;  // never expose the bridge to sub-frames
  const post = (s) => { )JS") +
           postExpression + QByteArrayLiteral(R"JS( };
  Object.defineProperty(window, "__materialmcNative", {
    value: Object.freeze({ post: (message) => post(String(message)) }),
    writable: false,
    configurable: false,
    enumerable: false,
  });
})();)JS");
}

QByteArray WebView::deliverScript(const QByteArray& json)
{
    // Serialise the payload as a JSON string literal ("...") so it can never break out of the call.
    QByteArray literal = QJsonDocument(QJsonArray{ QString::fromUtf8(json) }).toJson(QJsonDocument::Compact);
    literal = literal.mid(1, literal.size() - 2);  // strip [ ]
    return QByteArrayLiteral("window.__materialmcReceive && window.__materialmcReceive(") + literal + QByteArrayLiteral(");");
}

QByteArray WebView::contentSecurityPolicy()
{
    // - scripts/styles only from the bundle (Vite emits external files, React sets inline style attributes)
    // - images: bundle, launcher-served images, data: URIs, and remote https (mod platform icons)
    // - no network access from the page at all: connect-src 'none'; the page talks to the host via the bridge only
    return QByteArrayLiteral(
        "default-src 'none'; "
        "script-src 'self'; "
        "style-src 'self' 'unsafe-inline'; "
        "img-src 'self' data: https:; "
        "font-src 'self' data:; "
        "connect-src 'none'; "
        "frame-src 'none'; "
        "object-src 'none'; "
        "base-uri 'none'; "
        "form-action 'none'");
}

QByteArray WebView::mimeTypeFor(const QString& path)
{
    const auto lower = path.toLower();
    struct Entry {
        const char* suffix;
        const char* mime;
    };
    static constexpr Entry s_types[] = {
        { ".html", "text/html; charset=utf-8" },
        { ".js", "text/javascript; charset=utf-8" },
        { ".mjs", "text/javascript; charset=utf-8" },
        { ".css", "text/css; charset=utf-8" },
        { ".json", "application/json" },
        { ".svg", "image/svg+xml" },
        { ".png", "image/png" },
        { ".jpg", "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".gif", "image/gif" },
        { ".webp", "image/webp" },
        { ".ico", "image/x-icon" },
        { ".woff2", "font/woff2" },
        { ".woff", "font/woff" },
        { ".ttf", "font/ttf" },
        { ".txt", "text/plain; charset=utf-8" },
    };
    for (const auto& entry : s_types) {
        if (lower.endsWith(QLatin1String(entry.suffix))) {
            return entry.mime;
        }
    }
    return "application/octet-stream";
}

}  // namespace webview
