// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 *
 */
#pragma once

#include <QImage>
#include <QLoggingCategory>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>
#include <QPluginLoader>

#include <memory>

#include "api/PluginInstance.h"

Q_DECLARE_LOGGING_CATEGORY(pluginLogC)

class PluginInterface;
class Plugin;
class PluginContribution;

using PluginContributionPtr = std::shared_ptr<PluginContribution>;
using PluginPtr = std::shared_ptr<Plugin>;

class Plugin : public PluginInstance {
    Q_DISABLE_COPY(Plugin)
   public:
    Plugin() = default;
    Plugin(const QFileInfo& file);
    Plugin(QString file_path) : Plugin(QFileInfo(file_path)) {}
    ~Plugin() override;

    bool loadInfo();

    [[nodiscard]] auto name() const -> QString { return m_name; }
    [[nodiscard]] auto version() const -> QString { return m_version; }
    [[nodiscard]] auto description() const -> QString { return m_desc; }
    [[nodiscard]] auto homepage() const -> QString { return m_homepage; }
    [[nodiscard]] auto issueTracker() const -> QString { return m_issueTracker; }
    [[nodiscard]] auto authors() const -> QStringList { return m_authors; }
    [[nodiscard]] auto license() const -> QString { return m_license; }
    [[nodiscard]] bool enabled() const { return m_enabled; }
    [[nodiscard]] bool needsRestart() const { return m_needsRestart; }

    [[nodiscard]] QString iconPath() const { return m_icon_file; }
    [[nodiscard]] QPixmap icon(QSize size, Qt::AspectRatioMode mode = Qt::AspectRatioMode::IgnoreAspectRatio) const;
    void setIcon(QImage new_image) const;

    [[nodiscard]] QString relativePath(QString path) const override;

    enum class EnableAction { ENABLE, DISABLE, TOGGLE };
    void enable(EnableAction action);

    void onEnable();
    void onDisable();

   private:
    void loadV1(const QJsonObject& root);
    QString getNativePluginPath();

    QString m_name, m_version, m_desc, m_homepage, m_icon_file, m_issueTracker, m_license;
    QStringList m_authors;
    struct {
        QString osx;
        QString win32, win64;
        QString lin32, lin64;
    } m_native_plugin_paths;

    bool m_enabled = true, m_needsRestart = false;
    QList<PluginContributionPtr> m_contributions;
    QPluginLoader* m_loader = nullptr;
    PluginInterface* m_interface = nullptr;
    mutable QMutex m_data_lock;
    struct {
        QPixmapCache::Key key;
        bool was_ever_used = false;
        bool was_read_attempt = false;
    } mutable m_image_cache_key;
};
