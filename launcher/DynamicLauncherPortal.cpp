// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 utophii <pos18411@gmail.com>
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

#include "DynamicLauncherPortal.h"

#include <BuildConfig.h>

#include <QDebug>
#include <QEventLoop>
#include <QObject>
#include <QRegularExpression>
#include <QTimer>
#include <QVariantMap>

#ifdef WITH_QTDBUS
#include <QtDBus/QtDBus>
#endif

struct PortalBytesIcon {
    QString type;
    QByteArray data;
};

Q_DECLARE_METATYPE(PortalBytesIcon)

// (sv) = ("bytes", v: ay)
QDBusArgument& operator<<(QDBusArgument& arg, const PortalBytesIcon& icon)
{
    arg.beginStructure();
    arg << icon.type;
    arg << QDBusVariant(QVariant::fromValue(icon.data));
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>>(const QDBusArgument& arg, PortalBytesIcon& icon)
{
    arg.beginStructure();
    arg >> icon.type;
    QVariant data;
    arg >> data;
    icon.data = data.toByteArray();
    arg.endStructure();
    return arg;
}

namespace {

/// A QObject subclass needed to receive the D-Bus Response signal from the portal
class PortalResponseReceiver : public QObject {
    Q_OBJECT
   public:
    PortalResponseReceiver(QEventLoop& loop, QString& outToken, bool& outAccepted)
        : m_loop(loop), m_outToken(outToken), m_outAccepted(outAccepted)
    {}

   private slots:
    void portalResponse(uint responseCode, QVariantMap results)
    {
        m_outAccepted = (responseCode == 0);
        if (m_outAccepted)
            m_outToken = results.value(QStringLiteral("token")).toString();
        m_loop.quit();
    }

   private:
    QEventLoop& m_loop;
    QString& m_outToken;
    bool& m_outAccepted;
};

}  // namespace

namespace DynamicLauncherPortal {

static const QString PORTAL_SERVICE = QStringLiteral("org.freedesktop.portal.Desktop");
static const QString PORTAL_OBJECT_PATH = QStringLiteral("/org/freedesktop/portal/desktop");
static const QString PORTAL_INTERFACE = QStringLiteral("org.freedesktop.portal.DynamicLauncher");
static const QString REQUEST_INTERFACE = QStringLiteral("org.freedesktop.portal.Request");

bool isPortalAvailable()
{
#ifdef WITH_QTDBUS
    if (!QDBusConnection::sessionBus().isConnected())
        return false;

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QDBusConnection::sessionBus());
    return portal.isValid();
#else
    return false;
#endif
}

QString buildDesktopFileId(const QString& name)
{
    QString appId = BuildConfig.LAUNCHER_APPID;
    QString safeName = name;
    safeName.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_\\-.]")), QStringLiteral("_"));

    if (appId.endsWith('.'))
        return appId + safeName + ".desktop";
    return appId + "." + safeName + ".desktop";
}

Result<> installLauncher(const QString& name, const QByteArray& icon, const QString& desktopEntry)
{
#ifdef WITH_QTDBUS
    if (!QDBusConnection::sessionBus().isConnected()) {
        return std::unexpected{ QStringLiteral("D-Bus session bus not available") };
    }

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QDBusConnection::sessionBus());
    if (!portal.isValid()) {
        return std::unexpected{ QStringLiteral("Portal interface not available") };
    }

    // Register the custom icon type once (idempotent, thread-safe static init).
    // qDBusRegisterMetaType<T>() derives the D-Bus signature from the streaming
    // operators; here the signature is "(sv)"
    qDBusRegisterMetaType<PortalBytesIcon>();

    // Build the serialized GBytesIcon: (sv) = ("bytes", v: ay[data])
    PortalBytesIcon iconV;
    iconV.type = QStringLiteral("bytes");
    iconV.data = icon;

    // Call PrepareInstall - this shows a dialog to the user
    QDBusMessage prepareCall =
        QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QStringLiteral("PrepareInstall"));

    QVariantMap prepareOptions;
    prepareOptions[QStringLiteral("editable_name")] = false;
    prepareOptions[QStringLiteral("editable_icon")] = false;
    prepareOptions[QStringLiteral("launcher_type")] = 1u;  // 1 = Application

    // icon_v must be a D-Bus variant (v) containing the (sv) struct
    prepareCall << QString()                                                      // parent_window (empty = no parent window)
                << name                                                           // name
                << QVariant::fromValue(QDBusVariant(QVariant::fromValue(iconV)))  // icon_v
                << QVariant::fromValue(prepareOptions);                           // options

    QDBusMessage prepareReply = QDBusConnection::sessionBus().call(prepareCall, QDBus::BlockWithGui, 30000);

    if (prepareReply.type() == QDBusMessage::ErrorMessage) {
        return std::unexpected{ QStringLiteral("PrepareInstall failed: %1").arg(prepareReply.errorMessage()) };
    }

    if (prepareReply.arguments().isEmpty()) {
        return std::unexpected{ QStringLiteral("PrepareInstall returned no arguments") };
    }

    QDBusObjectPath handle = prepareReply.arguments().at(0).value<QDBusObjectPath>();
    qDebug() << "DynamicLauncherPortal: Got handle path:" << handle.path();

    // Set up the Request interface to listen for the Response signal
    QDBusInterface requestIface(PORTAL_SERVICE, handle.path(), REQUEST_INTERFACE, QDBusConnection::sessionBus());
    if (!requestIface.isValid()) {
        return std::unexpected{ QStringLiteral("Could not create Request interface") };
    }

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QString receivedToken;
    bool userAccepted = false;

    PortalResponseReceiver receiver(loop, receivedToken, userAccepted);

    // Connect to the Response signal using Qt SIGNAL/SLOT macros (works with MOC)
    QMetaObject::Connection signalConn =
        QObject::connect(&requestIface, SIGNAL(Response(uint, QVariantMap)), &receiver, SLOT(portalResponse(uint, QVariantMap)));
    if (!signalConn) {
        return std::unexpected{ QStringLiteral("Failed to connect to Response signal") };
    }

    // Connect timeout using the 4-arg QObject::connect with context
    QMetaObject::Connection timeoutConn = QObject::connect(&timeoutTimer, &QTimer::timeout, &receiver, [&userAccepted, &loop]() {
        userAccepted = false;
        loop.quit();
    });

    // Wait for user response
    timeoutTimer.start(300000);
    loop.exec();
    timeoutTimer.stop();

    // Disconnect both
    QObject::disconnect(signalConn);
    QObject::disconnect(timeoutConn);
    if (!userAccepted || receivedToken.isEmpty()) {
        qDebug() << "DynamicLauncherPortal: User did not accept";
        return std::unexpected{ QStringLiteral("User did not accept") };
    }

    const QString desktopFileId = buildDesktopFileId(name);

    // Call Install with the token
    QDBusMessage installCall =
        QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QStringLiteral("Install"));

    QVariantMap installOptions;
    installCall << receivedToken                         // token (s)
                << desktopFileId                         // desktop_file_id (s)
                << desktopEntry                          // desktop_entry (s)
                << QVariant::fromValue(installOptions);  // options (a{sv})

    QDBusMessage installReply = QDBusConnection::sessionBus().call(installCall, QDBus::BlockWithGui, 30000);

    if (installReply.type() == QDBusMessage::ErrorMessage) {
        return std::unexpected{ QStringLiteral("Install failed: %1").arg(installReply.errorMessage()) };
    }

    qDebug() << "DynamicLauncherPortal: Successfully installed launcher" << desktopFileId;
    return {};
#else
    Q_UNUSED(name);
    Q_UNUSED(icon);
    Q_UNUSED(desktopEntry);
    qWarning() << "DynamicLauncherPortal: Qt DBus support not compiled in";
    return std::unexpected{ QStringLiteral("Qt DBus support not compiled in") };
#endif
}

Result<> uninstallLauncher(const QString& desktopFileId)
{
#ifdef WITH_QTDBUS
    if (!QDBusConnection::sessionBus().isConnected()) {
        return std::unexpected{ QStringLiteral("D-Bus session bus not available") };
    }

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QDBusConnection::sessionBus());
    if (!portal.isValid()) {
        return std::unexpected{ QStringLiteral("Portal interface not available") };
    }

    QDBusMessage uninstallCall =
        QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QStringLiteral("Uninstall"));

    QVariantMap options;
    uninstallCall << desktopFileId << QVariant::fromValue(options);

    QDBusMessage uninstallReply = QDBusConnection::sessionBus().call(uninstallCall, QDBus::BlockWithGui, 30000);
    if (uninstallReply.type() == QDBusMessage::ErrorMessage) {
        return std::unexpected{ QStringLiteral("Uninstall failed: %1").arg(uninstallReply.errorMessage()) };
    }
    qDebug() << "DynamicLauncherPortal: Successfully uninstalled launcher" << desktopFileId;
    return {};
#else
    Q_UNUSED(desktopFileId);
    qWarning() << "DynamicLauncherPortal: Qt DBus support not compiled in";
    return std::unexpected{ QStringLiteral("Qt DBus support not compiled in") };
#endif
}

}  // namespace DynamicLauncherPortal

#include "DynamicLauncherPortal.moc"