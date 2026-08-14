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
    explicit ConfigHolder(QString path) : m_path(std::move(path))
    {
        m_saveTimer.setInterval(5000);
        m_saveTimer.setSingleShot(true);
        connect(&m_saveTimer, &QTimer::timeout, this, [this] { m_config.save(m_path); });
    }

    ~ConfigHolder() override
    {
        const bool applicableUpdate = m_update.has_value() && !m_update->skipSave && m_update->prev != m_config;
        if (applicableUpdate || m_saveTimer.isActive()) {
            m_config.save(m_path);
        }
    }

    const T& operator*() const { return m_config; }

    const T* operator->() const { return &m_config; }

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

    // FIXME: weirdly interacts with the save timer
    // if a save is queued, the config will be overwritten with an older version
    // this creates a bug where launching discards recently changed settings
    bool reload()
    {
        auto newConfig = T::load(m_path);
        if (!newConfig.has_value()) {
            return false;
        }

        m_config = std::move(newConfig.value());
        return true;
    }

    /// Saves immediately, and supresses pending saves.
    bool save()
    {
        m_saveTimer.stop();
        if (m_update.has_value()) {
            // NOTE: supressing the save rather than the whole updated signal, to avoid any surprises
            m_update->skipSave = true;
        }
        return m_config.save(m_path);
    }

   private:
    void markDirty()
    {
        if (m_update.has_value()) {
            // already waiting for handleDirty to be called
            // NOTE: unsupress the saving, because whilst it has been done explicitly there are still some
            m_update->skipSave = false;
            return;
        }

        m_update = { .prev = m_config, .skipSave = false };
        QMetaObject::invokeMethod(this, [this] { handleDirty(); }, Qt::QueuedConnection);
    }

    void handleDirty()
    {
        auto resetUpdate = qScopeGuard([this] { m_update = std::nullopt; });

        if (m_update->prev == m_config) {
            return;
        }

        emit updated();

        if (m_update->skipSave) {
            return;
        }

        if (m_saveTimer.isActive()) {
            qDebug() << u"Delaying config at" << m_path << "to be saved in" << m_saveTimer.interval() << u"ms";
        } else {
            qDebug() << u"Scheduling config at" << m_path << "to be saved in" << m_saveTimer.interval() << u"ms";
        }
        m_saveTimer.start();
    }

    QString m_path;
    T m_config;
    struct Update {
        T prev;
        bool skipSave;
    };
    std::optional<Update> m_update;
    QTimer m_saveTimer;
};
