// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Requiem Mod Launcher - Minecraft Launcher
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
#include "DiscordPresence.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QtEndian>

// Discord IPC wire protocol opcodes
static constexpr quint32 DISCORD_OP_HANDSHAKE = 0;
static constexpr quint32 DISCORD_OP_FRAME = 1;
static constexpr quint32 DISCORD_OP_CLOSE = 2;

// Retry connecting to Discord every 15 seconds
static constexpr int RECONNECT_INTERVAL_MS = 15'000;

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

DiscordPresence::DiscordPresence(const QString& clientId, QObject* parent) : QObject(parent), m_clientId(clientId)
{
    m_socket = new QLocalSocket(this);
    connect(m_socket, &QLocalSocket::connected, this, &DiscordPresence::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &DiscordPresence::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &DiscordPresence::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &DiscordPresence::onSocketError);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DiscordPresence::tryConnect);

    if (!m_clientId.isEmpty()) {
        tryConnect();
        m_reconnectTimer->start();
    }
}

DiscordPresence::~DiscordPresence()
{
    m_reconnectTimer->stop();
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        clearPresence();
        m_socket->disconnectFromServer();
        m_socket->waitForDisconnected(500);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void DiscordPresence::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (!enabled) {
        clearPresence();
        m_reconnectTimer->stop();
    } else if (!m_clientId.isEmpty()) {
        tryConnect();
        m_reconnectTimer->start();
    }
}

void DiscordPresence::setIdlePresence()
{
    if (!m_enabled)
        return;

    QJsonObject assets;
    assets["large_image"] = "requiem";
    assets["large_text"] = "Requiem Mod Launcher";

    QJsonObject activity;
    activity["details"] = "In the Launcher";
    activity["state"] = "Browsing Instances";
    activity["assets"] = assets;

    setActivity(activity);
}

void DiscordPresence::setPlayingPresence(const QString& instanceName, const QString& mcVersion, qint64 startTimeSecs)
{
    if (!m_enabled)
        return;

    QJsonObject assets;
    assets["large_image"] = "requiem";
    assets["large_text"] = "Requiem Mod Launcher";
    assets["small_image"] = "minecraft";
    assets["small_text"] = mcVersion.isEmpty() ? "Minecraft" : "Minecraft " + mcVersion;

    QJsonObject timestamps;
    timestamps["start"] = startTimeSecs;

    QJsonArray buttons;
    QJsonObject button;
    button["label"] = "Get Requiem";
    button["url"] = "https://github.com/Houars/Requiem";
    buttons.append(button);

    QJsonObject activity;
    activity["details"] = instanceName.isEmpty() ? "Playing Minecraft" : "Playing \u25B6 " + instanceName;
    activity["state"] = mcVersion.isEmpty() ? "Minecraft" : "Minecraft " + mcVersion;
    activity["timestamps"] = timestamps;
    activity["assets"] = assets;
    activity["buttons"] = buttons;

    setActivity(activity);
}

void DiscordPresence::clearPresence()
{
    if (!m_handshakeDone)
        return;

    QJsonObject args;
    args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
    // Omitting "activity" key sends a null activity, which clears presence

    QJsonObject cmd;
    cmd["cmd"] = "SET_ACTIVITY";
    cmd["nonce"] = QString::number(++m_nonce);
    cmd["args"] = args;

    sendFrame(DISCORD_OP_FRAME, QJsonDocument(cmd).toJson(QJsonDocument::Compact));
}

// ─────────────────────────────────────────────────────────────────────────────
// Connection / socket path discovery
// ─────────────────────────────────────────────────────────────────────────────

QStringList DiscordPresence::socketCandidates()
{
    QStringList paths;
    auto env = QProcessEnvironment::systemEnvironment();

#if defined(Q_OS_WIN)
    // Qt's QLocalSocket prepends \\.\pipe\ automatically on Windows
    for (int i = 0; i < 10; ++i)
        paths << QStringLiteral("discord-ipc-%1").arg(i);

#elif defined(Q_OS_MACOS)
    QString tmpDir = env.value(QStringLiteral("TMPDIR"));
    while (tmpDir.endsWith('/'))
        tmpDir.chop(1);
    if (!tmpDir.isEmpty())
        for (int i = 0; i < 10; ++i)
            paths << tmpDir + QStringLiteral("/discord-ipc-%1").arg(i);
    for (int i = 0; i < 10; ++i)
        paths << QStringLiteral("/tmp/discord-ipc-%1").arg(i);

#else
    // Linux and other Unix: prefer XDG_RUNTIME_DIR, then /tmp
    QString xdg = env.value(QStringLiteral("XDG_RUNTIME_DIR"));
    if (!xdg.isEmpty()) {
        for (int i = 0; i < 10; ++i)
            paths << xdg + QStringLiteral("/discord-ipc-%1").arg(i);
        // Flatpak Discord uses a subdirectory
        for (int i = 0; i < 10; ++i)
            paths << xdg + QStringLiteral("/app/com.discordapp.Discord/discord-ipc-%1").arg(i);
    }
    for (int i = 0; i < 10; ++i)
        paths << QStringLiteral("/tmp/discord-ipc-%1").arg(i);
#endif

    return paths;
}

void DiscordPresence::tryConnect()
{
    if (!m_enabled || m_clientId.isEmpty() || m_scanning)
        return;
    if (m_socket->state() != QLocalSocket::UnconnectedState)
        return;

    m_scanning = true;
    m_pathIndex = 0;
    attemptNextPath();
}

void DiscordPresence::attemptNextPath()
{
    if (!m_scanning)
        return;

    const auto paths = socketCandidates();
    if (m_pathIndex >= paths.size()) {
        m_scanning = false;
        return;
    }
    m_socket->connectToServer(paths[m_pathIndex]);
}

// ─────────────────────────────────────────────────────────────────────────────
// QLocalSocket signal handlers
// ─────────────────────────────────────────────────────────────────────────────

void DiscordPresence::onConnected()
{
    m_scanning = false;
    m_handshakeDone = false;
    m_readBuffer.clear();
    sendHandshake();
}

void DiscordPresence::onDisconnected()
{
    m_scanning = false;
    m_handshakeDone = false;
    m_readBuffer.clear();
}

void DiscordPresence::onSocketError(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error);
    // If we were scanning candidate paths and the current one failed, try the next
    if (m_scanning && m_socket->state() == QLocalSocket::UnconnectedState) {
        m_pathIndex++;
        QTimer::singleShot(0, this, &DiscordPresence::attemptNextPath);
    } else {
        m_scanning = false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// IPC framing
// ─────────────────────────────────────────────────────────────────────────────

void DiscordPresence::sendHandshake()
{
    QJsonObject obj;
    obj["v"] = 1;
    obj["client_id"] = m_clientId;
    sendFrame(DISCORD_OP_HANDSHAKE, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void DiscordPresence::sendFrame(quint32 opcode, const QByteArray& payload)
{
    if (m_socket->state() != QLocalSocket::ConnectedState)
        return;

    const quint32 len = static_cast<quint32>(payload.size());

    // 8-byte little-endian header: [opcode (4)] [length (4)]
    quint8 header[8];
    qToLittleEndian(opcode, header);
    qToLittleEndian(len, header + 4);

    m_socket->write(reinterpret_cast<const char*>(header), 8);
    m_socket->write(payload);
    m_socket->flush();
}

void DiscordPresence::onReadyRead()
{
    m_readBuffer.append(m_socket->readAll());

    // Parse all complete frames from the buffer
    while (m_readBuffer.size() >= 8) {
        const auto* raw = reinterpret_cast<const uchar*>(m_readBuffer.constData());
        const quint32 opcode = qFromLittleEndian<quint32>(raw);
        const quint32 length = qFromLittleEndian<quint32>(raw + 4);

        if (static_cast<quint32>(m_readBuffer.size()) < 8 + length)
            break;  // Incomplete frame — wait for more data

        const QByteArray frameData = m_readBuffer.mid(8, static_cast<int>(length));
        m_readBuffer.remove(0, static_cast<int>(8 + length));

        if (opcode == DISCORD_OP_FRAME) {
            const QJsonDocument doc = QJsonDocument::fromJson(frameData);
            if (!doc.isNull() && doc.isObject()) {
                const QString evt = doc.object().value("evt").toString();
                if (evt == QLatin1String("READY")) {
                    m_handshakeDone = true;
                    // Deliver any activity that was requested before the handshake completed
                    if (m_hasPendingActivity) {
                        m_hasPendingActivity = false;
                        setActivity(m_pendingActivity);
                    }
                }
            }
        } else if (opcode == DISCORD_OP_CLOSE) {
            m_socket->disconnectFromServer();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Activity helpers
// ─────────────────────────────────────────────────────────────────────────────

void DiscordPresence::setActivity(const QJsonObject& activity)
{
    if (!m_enabled)
        return;

    if (!m_handshakeDone) {
        // Buffer the request; it will be sent once the handshake completes
        m_hasPendingActivity = true;
        m_pendingActivity = activity;
        // Kick off a connection attempt if not already connected/scanning
        if (m_socket->state() == QLocalSocket::UnconnectedState && !m_clientId.isEmpty())
            tryConnect();
        return;
    }

    QJsonObject args;
    args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
    args["activity"] = activity;

    QJsonObject cmd;
    cmd["cmd"] = "SET_ACTIVITY";
    cmd["nonce"] = QString::number(++m_nonce);
    cmd["args"] = args;

    sendFrame(DISCORD_OP_FRAME, QJsonDocument(cmd).toJson(QJsonDocument::Compact));
}
