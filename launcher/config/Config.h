// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
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

#include <QMetaObject>
#include <QObject>
#include <QTimer>
#include <concepts>
#include <optional>

/// Represents a type which can be used in ConfigHolder since it supports saving, loading and comparison.
template <typename T>
concept ConfigObject = requires(T x) {
    { T::load(QString()) } -> std::same_as<std::optional<T>>;
    { x.save(QString()) } -> std::same_as<bool>;
    { x == x } -> std::same_as<bool>;
    { x != x } -> std::same_as<bool>;
    { T(x) };
    { x = x };
};

// HACK: qt won't allow signals on a template
class ConfigHolderSignals : public QObject {
    Q_OBJECT
   signals:
    /// Emitted when the config is updated.
    /// Call prev() to access the previous config.
    void updated();
};

/// Wrapper for ConfigObject which automatically handles saving and emits a signal on update.
/// Should only be called from one thread!
template <ConfigObject T>
class ConfigHolder : public ConfigHolderSignals {
   public:
    /// Creates a ConfigHolder with the specified path for reading and writing.
    explicit ConfigHolder(QString path, const T& config) : m_path(std::move(path)), m_config(config)
    {
        m_saveTimer.setInterval(5000);
        m_saveTimer.setSingleShot(true);
        connect(&m_saveTimer, &QTimer::timeout, this, [this] { m_config.save(m_path); });
    }

    ~ConfigHolder() override
    {
        if (savePending()) {
            m_config.save(m_path);
        }
    }

    const T& operator*() const { return m_config; }

    const T* operator->() const { return &m_config; }

    /// Marks the config as dirty before returning it.
    /// If changes have actually been made, updated will be emitted.
    T& update()
    {
        markDirty();
        return m_config;
    }

    /// Returns the previous configuration.
    /// When calling update, it is held until after the updated signal has been emitted.
    /// It is only recommended to call this inside a handler for updated.
    /// Be careful to make sure your slot is not using Qt::QueuedConnection.
    std::optional<T> prev() const
    {
        if (!m_update.has_value()) {
            return std::nullopt;
        }
        return m_update->prev;
    }

    bool savePending() const
    {
        if (m_update.has_value() && m_update->onDisk != m_config) {
            return true;
        }
        if (m_saveTimer.isActive()) {
            return true;
        }

        return false;
    }

    /// Reloads the contents of the config.
    /// It is recommended not to call this if savePending() is true, since recent changed which have not been saved yet may get overriden.
    [[nodiscard]] bool reload()
    {
        auto newConfig = T::load(m_path);
        if (!newConfig.has_value()) {
            return false;
        }

        // NOTE: we want the changed signal to be emitted but a save not to be queued unless further changes are made
        m_saveTimer.stop();
        markDirty();

        m_config = std::move(newConfig.value());
        // NOTE: avoids a save being queued
        m_update->onDisk = m_config;

        return true;
    }

    /// Saves immediately, and supresses pending saves.
    bool save()
    {
        if (!m_config.save(m_path)) {
            return false;
        }

        m_saveTimer.stop();
        if (m_update.has_value()) {
            m_update->onDisk = m_config;
        }
        return true;
    }

   private:
    void markDirty()
    {
        if (m_update.has_value()) {
            // already waiting for handleDirty to be called
            return;
        }

        m_update = { .prev = m_config, .onDisk = m_config };
        QMetaObject::invokeMethod(this, [this] { handleDirty(); }, Qt::QueuedConnection);
    }

    void handleDirty()
    {
        auto resetUpdate = qScopeGuard([this] { m_update = std::nullopt; });

        if (m_update->prev != m_config) {
            emit updated();
        }

        if (m_update->onDisk != m_config) {
            if (m_saveTimer.isActive()) {
                qDebug() << u"Delaying config at" << m_path << "to be saved in" << m_saveTimer.interval() << u"ms";
            } else {
                qDebug() << u"Scheduling config at" << m_path << "to be saved in" << m_saveTimer.interval() << u"ms";
            }
            m_saveTimer.start();
        }
    }

    QString m_path;
    T m_config;
    struct Update {
        T prev;
        T onDisk;
    };
    std::optional<Update> m_update;
    QTimer m_saveTimer;
};
