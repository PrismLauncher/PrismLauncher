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
#pragma once

#include <QJsonObject>
#include <QLocalSocket>
#include <QObject>
#include <QString>
#include <QTimer>

/**
 * @brief Manages Discord Rich Presence via the Discord IPC protocol.
 *
 * Connects to the local Discord client via a Unix socket (Linux/macOS)
 * or named pipe (Windows) and sends SET_ACTIVITY frames to display
 * launcher and gameplay state in the user's Discord profile.
 *
 * To enable:
 *   1. Register a Discord application at https://discord.com/developers/applications
 *   2. Upload rich presence assets named "requiem" and "minecraft"
 *   3. Pass the Application ID to the constructor (or set Launcher_DISCORD_CLIENT_ID in cmake)
 */
class DiscordPresence : public QObject {
    Q_OBJECT
   public:
    /**
     * @param clientId  Discord Application ID. Pass empty string to disable.
     * @param parent    QObject parent.
     */
    explicit DiscordPresence(const QString& clientId, QObject* parent = nullptr);
    ~DiscordPresence() override;

    /** Enable or disable the Rich Presence integration at runtime. */
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    /** Show the "browsing instances" idle state. */
    void setIdlePresence();

    /**
     * @brief Show the "playing Minecraft" state.
     * @param instanceName  Display name of the launched instance.
     * @param mcVersion     Minecraft version string (may be empty).
     * @param startTimeSecs Unix timestamp when the session began.
     */
    void setPlayingPresence(const QString& instanceName, const QString& mcVersion, qint64 startTimeSecs);

    /** Remove the Rich Presence activity entirely. */
    void clearPresence();

   private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QLocalSocket::LocalSocketError error);
    void tryConnect();
    void attemptNextPath();

   private:
    void sendHandshake();
    void sendFrame(quint32 opcode, const QByteArray& payload);
    void setActivity(const QJsonObject& activity);

    /** Returns ordered candidate socket paths to try when connecting. */
    static QStringList socketCandidates();

    QString m_clientId;
    bool m_enabled = true;
    bool m_handshakeDone = false;

    // Connection scanning state: cycles through socketCandidates() asynchronously
    bool m_scanning = false;
    int m_pathIndex = 0;

    QLocalSocket* m_socket = nullptr;
    QTimer* m_reconnectTimer = nullptr;

    // Buffer for incoming IPC frames
    QByteArray m_readBuffer;

    // Activity buffered before the handshake completes
    bool m_hasPendingActivity = false;
    QJsonObject m_pendingActivity;

    int m_nonce = 0;
};
