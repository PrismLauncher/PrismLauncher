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

#include <QHash>
#include <QObject>
#include <QString>

#include <memory>

namespace webview {
class WebView;
struct Response;
}  // namespace webview

namespace api {
class AccountApi;
class ApiRouter;
class ConsoleApi;
class InstanceApi;
class ModApi;
class ResourceApi;
class SettingsApi;
class TaskTracker;
}  // namespace api

/**
 * The React UI of MaterialMC: one native window with the system WebView, the RPC router and every API module.
 *
 * Content comes from the bundled `frontend/` directory through `materialmc://app/`, or — in development builds
 * only — from the URL in MATERIALMC_DEV_URL (e.g. the Vite dev server at http://localhost:5173).
 */
class WebUiHost : public QObject {
    Q_OBJECT
   public:
    /** Creates and shows the web UI. Returns nullptr (and sets `error`) if no WebView or bundle is available. */
    static std::unique_ptr<WebUiHost> create(QString* error);
    ~WebUiHost() override;

    void present();
    void hide();

   private:
    WebUiHost() = default;
    bool init(QString* error);
    webview::Response serve(const QString& path);
    webview::Response serveStatic(const QString& path) const;
    webview::Response serveImage(const QStringList& segments);

    /** Locates the built frontend (a directory containing index.html). */
    static QString findFrontendRoot();

    QString m_root;
    QString m_devUrl;
    std::unique_ptr<webview::WebView> m_view;
    std::unique_ptr<api::ApiRouter> m_router;
    std::unique_ptr<api::TaskTracker> m_tasks;
    std::unique_ptr<api::InstanceApi> m_instances;
    std::unique_ptr<api::ConsoleApi> m_console;
    std::unique_ptr<api::ResourceApi> m_resources;
    std::unique_ptr<api::AccountApi> m_accounts;
    std::unique_ptr<api::ModApi> m_mods;
    std::unique_ptr<api::SettingsApi> m_settings;
    QObject m_context;
    QHash<QString, QByteArray> m_imageCache;
};
