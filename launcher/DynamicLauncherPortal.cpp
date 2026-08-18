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
#include <QFile>
#include <QRegularExpression>
#include <QTimer>
#include <QVariantMap>

#ifdef WITH_QTDBUS
#include <QtDBus/QtDBus>
#endif

struct PortalBytesIcon
{
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

bool installLauncher(const QString& name, const QString& iconPath, const QString& desktopEntry)
{
#ifdef WITH_QTDBUS
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning() << "DynamicLauncherPortal: D-Bus session bus not available";
        return false;
    }

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QDBusConnection::sessionBus());
    if (!portal.isValid()) {
        qWarning() << "DynamicLauncherPortal: Portal interface not available";
        return false;
    }

    // Register the custom icon type once (idempotent, thread-safe static init).
    // qDBusRegisterMetaType<T>() derives the D-Bus signature from the streaming
    // operators; here the signature is "(sv)"
    qDBusRegisterMetaType<PortalBytesIcon>();

    // Read the icon file
    QByteArray iconData;
    QFile iconFile(iconPath);
    if (iconFile.open(QIODevice::ReadOnly)) {
        iconData = iconFile.readAll();
        iconFile.close();
        qDebug() << "DynamicLauncherPortal: Read icon from" << iconPath << "(" << iconData.size() << "bytes)";
    } else {
        qWarning() << "DynamicLauncherPortal: Could not read icon file" << iconPath;
    }

    // Build the serialized GBytesIcon: (sv) = ("bytes", v: ay[data])
    PortalBytesIcon icon;
    icon.type = QStringLiteral("bytes");
    icon.data = iconData;

    // Call PrepareInstall - this shows a dialog to the user
    QDBusMessage prepareCall = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QStringLiteral("PrepareInstall"));

    QVariantMap prepareOptions;
    prepareOptions[QStringLiteral("editable_name")] = false;
    prepareOptions[QStringLiteral("editable_icon")] = false;
    prepareOptions[QStringLiteral("launcher_type")] = 1u;  // 1 = Application

    // icon_v must be a D-Bus variant (v) containing the (sv) struct
    prepareCall << QString()  // parent_window (empty = no parent window)
                << name          // name
                << QVariant::fromValue(QDBusVariant(QVariant::fromValue(icon)))  // icon_v
                << QVariant::fromValue(prepareOptions);  // options

    QDBusMessage prepareReply = QDBusConnection::sessionBus().call(prepareCall, QDBus::BlockWithGui, 30000);

    if (prepareReply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "DynamicLauncherPortal: PrepareInstall failed:" << prepareReply.errorMessage();
        return false;
    }

    if (prepareReply.arguments().isEmpty()) {
        qWarning() << "DynamicLauncherPortal: PrepareInstall returned no arguments";
        return false;
    }

    QDBusObjectPath handle = prepareReply.arguments().at(0).value<QDBusObjectPath>();
    qDebug() << "DynamicLauncherPortal: Got handle path:" << handle.path();

    // Set up the Request interface to listen for the Response signal
    QDBusInterface requestIface(PORTAL_SERVICE, handle.path(), REQUEST_INTERFACE, QDBusConnection::sessionBus());
    if (!requestIface.isValid()) {
        qWarning() << "DynamicLauncherPortal: Could not create Request interface";
        return false;
    }

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QString receivedToken;
    bool userAccepted = false;

    PortalResponseReceiver receiver;
    receiver.loop = &loop;
    receiver.outToken = &receivedToken;
    receiver.outAccepted = &userAccepted;

    // Connect to the Response signal using Qt SIGNAL/SLOT macros (works with MOC)
    QMetaObject::Connection signalConn = QObject::connect(
        &requestIface,
        SIGNAL(Response(uint,QVariantMap)),
        &receiver, SLOT(portalResponse(uint,QVariantMap))
    );
    if (!signalConn) {
        qWarning() << "DynamicLauncherPortal: Failed to connect to Response signal";
        return false;
    }

    // Connect timeout using the 4-arg QObject::connect with context
    QMetaObject::Connection timeoutConn = QObject::connect(
        &timeoutTimer, &QTimer::timeout,
        &receiver,
        [&userAccepted, &loop]() {
            userAccepted = false;
            loop.quit();
        }
    );

    // Wait for user response
    timeoutTimer.start(300000);
    loop.exec();
    timeoutTimer.stop();

    // Disconnect both
    QObject::disconnect(signalConn);
    QObject::disconnect(timeoutConn);
    if (!userAccepted || receivedToken.isEmpty()) {
        qDebug() << "DynamicLauncherPortal: User did not accept";
        return false;
    }

    // Build desktop_file_id
    QString appId = BuildConfig.LAUNCHER_APPID;
    QString safeName = name;
    safeName.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_\\-.]")), QStringLiteral("_"));

    QString desktopFileId;
    if (!appId.endsWith('.'))
        desktopFileId = appId + '.' + safeName + ".desktop";
    else
        desktopFileId = appId + safeName + ".desktop";

    // Call Install with the token
    QDBusMessage installCall = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QStringLiteral("Install"));

    QVariantMap installOptions;
    installCall << receivedToken                // token (s)
                << desktopFileId                 // desktop_file_id (s)
                << desktopEntry                  // desktop_entry (s)
                << QVariant::fromValue(installOptions);  // options (a{sv})

    QDBusMessage installReply = QDBusConnection::sessionBus().call(installCall, QDBus::BlockWithGui, 30000);

    if (installReply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "DynamicLauncherPortal: Install failed:" << installReply.errorMessage();
        return false;
    }

    qDebug() << "DynamicLauncherPortal: Successfully installed launcher" << desktopFileId;
    return true;
#else
    Q_UNUSED(name);
    Q_UNUSED(iconPath);
    Q_UNUSED(desktopEntry);
    qWarning() << "DynamicLauncherPortal: Qt DBus support not compiled in";
    return false;
#endif
}

bool uninstallLauncher(const QString& desktopFileId)
{
#ifdef WITH_QTDBUS
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning() << "DynamicLauncherPortal: D-Bus session bus not available";
        return false;
    }

    QDBusInterface portal(PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QDBusConnection::sessionBus());
    if (!portal.isValid()) {
        qWarning() << "DynamicLauncherPortal: Portal interface not available";
        return false;
    }

    QDBusMessage uninstallCall = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_OBJECT_PATH, PORTAL_INTERFACE, QStringLiteral("Uninstall"));

    QVariantMap options;
    uninstallCall << desktopFileId
                  << QVariant::fromValue(options);

    QDBusMessage uninstallReply = QDBusConnection::sessionBus().call(uninstallCall, QDBus::BlockWithGui, 30000);
    if (uninstallReply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "DynamicLauncherPortal: Uninstall failed:" << uninstallReply.errorMessage();
        return false;
    }
    qDebug() << "DynamicLauncherPortal: Successfully uninstalled launcher" << desktopFileId;
    return true;
#else
    Q_UNUSED(desktopFileId);
    qWarning() << "DynamicLauncherPortal: Qt DBus support not compiled in";
    return false;
#endif
}

} // namespace DynamicLauncherPortal