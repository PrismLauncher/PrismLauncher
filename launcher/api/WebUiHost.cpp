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

#include "WebUiHost.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QPixmap>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"
#include "webview/WebView.h"

#include "AccountApi.h"
#include "ApiRouter.h"
#include "ConsoleApi.h"
#include "InstanceApi.h"
#include "ModApi.h"
#include "ResourceApi.h"
#include "SettingsApi.h"
#include "SystemApi.h"
#include "TaskTracker.h"
#include "VersionApi.h"

namespace {

constexpr qint64 MaxServedFileSize = 64 * 1024 * 1024;

QByteArray encodePng(const QImage& image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

webview::Response pngResponse(const QByteArray& bytes)
{
    if (bytes.isEmpty()) {
        return webview::Response::notFound();
    }
    return { 200, "image/png", bytes };
}

webview::Response fileResponse(const QString& path)
{
    QFile file(path);
    if (file.size() > MaxServedFileSize || !file.open(QIODevice::ReadOnly)) {
        return webview::Response::notFound();
    }
    return { 200, {}, file.readAll() };
}

/** Accepts only http(s) URLs on the loopback interface: the dev server must never be a remote host. */
QString validatedDevUrl(const QString& raw)
{
    const QUrl url(raw);
    const auto host = url.host();
    const bool loopback = host == QLatin1String("localhost") || host == QLatin1String("127.0.0.1") || host == QLatin1String("::1");
    if (!url.isValid() || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")) || !loopback) {
        qWarning() << "Ignoring MATERIALMC_DEV_URL" << raw << "- only http(s)://localhost URLs are allowed";
        return {};
    }
    return url.toString();
}

}  // namespace

std::unique_ptr<WebUiHost> WebUiHost::create(QString* error)
{
    std::unique_ptr<WebUiHost> host(new WebUiHost());
    if (!host->init(error)) {
        return nullptr;
    }
    return host;
}

WebUiHost::~WebUiHost()
{
    APPLICATION->setLaunchInteraction(nullptr);
    // Destroy the page first so no callback reaches the API objects while they are torn down.
    m_view.reset();
}

QString WebUiHost::findFrontendRoot()
{
    const auto appDir = QCoreApplication::applicationDirPath();
    QStringList candidates;
#ifdef MATERIALMC_WEBUI_ALLOW_DEV_URL
    if (const auto overrideDir = qEnvironmentVariable("MATERIALMC_FRONTEND_DIR"); !overrideDir.isEmpty()) {
        candidates << overrideDir;
    }
#endif
    candidates << FS::PathCombine(appDir, "frontend");  // build tree, Windows, portable
#if defined(Q_OS_MACOS)
    candidates << FS::PathCombine(appDir, "..", "Resources", "frontend");  // app bundle
#else
    candidates << FS::PathCombine(APPLICATION->root(), "share", BuildConfig.LAUNCHER_NAME, "frontend")
               << FS::PathCombine(FS::PathCombine(appDir, ".."), "share", BuildConfig.LAUNCHER_NAME, "frontend");  // FHS install
#endif
    for (const auto& dir : candidates) {
        if (QFileInfo(FS::PathCombine(dir, "index.html")).isFile()) {
            return QFileInfo(dir).canonicalFilePath();
        }
    }
    return {};
}

bool WebUiHost::init(QString* error)
{
#ifdef MATERIALMC_WEBUI_ALLOW_DEV_URL
    if (const auto dev = qEnvironmentVariable("MATERIALMC_DEV_URL"); !dev.isEmpty()) {
        m_devUrl = validatedDevUrl(dev);
    }
    const bool devTools = !m_devUrl.isEmpty() || qEnvironmentVariableIntValue("MATERIALMC_WEBUI_DEVTOOLS") != 0;
#else
    const bool devTools = false;
#endif

    if (m_devUrl.isEmpty()) {
        m_root = findFrontendRoot();
        if (m_root.isEmpty()) {
            if (error) {
                *error = QObject::tr("The web UI bundle (frontend/index.html) was not found next to the launcher.");
            }
            return false;
        }
    }

    webview::Options options;
    options.title = BuildConfig.LAUNCHER_DISPLAYNAME;
    options.iconName = BuildConfig.LAUNCHER_APPID;
    options.developerTools = devTools;
    if (!m_devUrl.isEmpty()) {
        const QUrl dev(m_devUrl);
        options.devOrigin = QStringLiteral("%1://%2:%3").arg(dev.scheme(), dev.host()).arg(dev.port(dev.scheme() == "https" ? 443 : 80));
    }

    m_view = webview::WebView::create(options, error);
    if (!m_view) {
        return false;
    }

    m_router = std::make_unique<api::ApiRouter>();
    m_tasks = std::make_unique<api::TaskTracker>(m_router.get());
    m_instances = std::make_unique<api::InstanceApi>(m_router.get(), m_tasks.get());
    m_console = std::make_unique<api::ConsoleApi>(m_router.get(), m_instances.get());
    m_resources = std::make_unique<api::ResourceApi>(m_router.get(), m_tasks.get());
    m_accounts = std::make_unique<api::AccountApi>(m_router.get(), m_tasks.get());
    m_mods = std::make_unique<api::ModApi>(m_router.get(), m_tasks.get());
    m_settings = std::make_unique<api::SettingsApi>(m_router.get());
    api::registerSystemApi(m_router.get(), m_tasks.get(), { m_view->engineName(), !m_devUrl.isEmpty() });
    api::registerVersionApi(m_router.get(), m_tasks.get(), &m_context);

    APPLICATION->setLaunchInteraction(m_instances.get());
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, [this](const QString& key) { m_imageCache.remove(key); });

    auto* view = m_view.get();
    connect(m_router.get(), &api::ApiRouter::outgoing, this, [view](const QByteArray& json) { view->postMessage(json); });
    m_view->setMessageHandler([this](const QByteArray& message) { m_router->handleMessage(message); });
    m_view->setResourceHandler([this](const QString& path) { return serve(path); });
    m_view->setExternalUrlHandler([](const QUrl& url) { DesktopServices::openUrl(url); });
    m_view->setCloseHandler([] {
        // Hide rather than destroy: running games keep the launcher (and this host) alive.
        QMetaObject::invokeMethod(APPLICATION, [] { APPLICATION->webUiClosed(); }, Qt::QueuedConnection);
        return true;
    });

    QUrl url = m_devUrl.isEmpty() ? webview::WebView::appUrl() : QUrl(m_devUrl);
#ifdef MATERIALMC_WEBUI_ALLOW_DEV_URL
    // Development aid: open a given route directly, e.g. MATERIALMC_WEBUI_ROUTE=/settings
    if (const auto route = qEnvironmentVariable("MATERIALMC_WEBUI_ROUTE"); route.startsWith('/')) {
        url.setFragment(route);
    }
#endif
    qDebug() << "Web UI: loading" << url << (m_root.isEmpty() ? QString() : QStringLiteral("from %1").arg(m_root)) << "with"
             << m_view->engineName();
    m_view->load(url);
    m_view->show();
    return true;
}

void WebUiHost::present()
{
    m_view->present();
}

void WebUiHost::hide()
{
    m_view->hide();
}

webview::Response WebUiHost::serve(const QString& path)
{
    // Host-generated images live under "/_<kind>/..."; everything else is the static bundle.
    if (path.startsWith(QLatin1String("/_"))) {
        return serveImage(path.mid(1).split('/'));
    }
    return serveStatic(path);
}

webview::Response WebUiHost::serveStatic(const QString& path) const
{
    if (m_root.isEmpty()) {
        return webview::Response::notFound();
    }
    const QString relative = (path.isEmpty() || path == QLatin1String("/")) ? QStringLiteral("/index.html") : path;
    const QString candidate = QDir::cleanPath(m_root + relative);
    // Refuse anything that resolves outside the bundle (.., symlinks).
    const QFileInfo info(candidate);
    const auto canonical = info.canonicalFilePath();
    if (canonical.isEmpty() || !canonical.startsWith(m_root + '/') || !info.isFile()) {
        return webview::Response::notFound();
    }
    return fileResponse(canonical);
}

webview::Response WebUiHost::serveImage(const QStringList& segments)
{
    const auto kind = segments.value(0);
    if (kind == QLatin1String("_icon") && segments.size() == 2) {
        const auto& key = segments[1];
        if (!m_imageCache.contains(key)) {
            const auto icon = APPLICATION->icons()->getIcon(key);
            if (icon.isNull()) {
                return webview::Response::notFound();
            }
            m_imageCache.insert(key, encodePng(icon.pixmap(128, 128).toImage()));
        }
        return pngResponse(m_imageCache.value(key));
    }
    if (kind == QLatin1String("_face") && segments.size() == 2) {
        const auto account = api::AccountApi::findAccount(segments[1]);
        return account ? pngResponse(encodePng(account->getFace(64, 64).toImage())) : webview::Response::notFound();
    }
    if ((kind == QLatin1String("_screenshot") || kind == QLatin1String("_world")) && segments.size() == 3) {
        auto* instance = APPLICATION->instances()->getInstanceById(segments[1]);
        if (!instance) {
            return webview::Response::notFound();
        }
        const auto file =
            kind == QLatin1String("_screenshot") ? api::ResourceApi::screenshotPath(instance, segments[2]) : api::ResourceApi::worldIconPath(instance, segments[2]);
        return file.isEmpty() ? webview::Response::notFound() : fileResponse(file);
    }
    return webview::Response::notFound();
}
