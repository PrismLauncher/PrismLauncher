/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "LocalPeer.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QTime>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include "LockedFile.h"

#if defined(Q_OS_WIN)
#include <QLibrary>
#include <qt_windows.h>
typedef BOOL(WINAPI*PProcessIdToSessionId)(DWORD,DWORD*);
static PProcessIdToSessionId pProcessIdToSessionId = 0;
#endif
#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <unistd.h>
#endif

#include <chrono>
#include <thread>
#include <QCryptographicHash>

static const char* ack = "ack";

ApplicationId ApplicationId::fromTraditionalApp()
{
    QString protoId = QCoreApplication::applicationFilePath();
#if defined(Q_OS_WIN)
    protoId = protoId.toLower();
#endif
    auto prefix = protoId.section(QLatin1Char('/'), -1);
    prefix.remove(QRegExp("[^a-zA-Z]"));
    prefix.truncate(6);
    QByteArray idc = protoId.toUtf8();
    quint16 idNum = qChecksum(idc.constData(), idc.size());
    auto socketName = QLatin1String("qtsingleapp-") + prefix + QLatin1Char('-') + QString::number(idNum, 16);
#if defined(Q_OS_WIN)
    if (!pProcessIdToSessionId)
    {
        QLibrary lib("kernel32");
        pProcessIdToSessionId = (PProcessIdToSessionId)lib.resolve("ProcessIdToSessionId");
    }
    if (pProcessIdToSessionId)
    {
        DWORD sessionId = 0;
        pProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
        socketName += QLatin1Char('-') + QString::number(sessionId, 16);
    }
#else
    socketName += QLatin1Char('-') + QString::number(::getuid(), 16);
#endif
    return ApplicationId(socketName);
}

ApplicationId ApplicationId::fromPathAndVersion(const QString& dataPath, const QString& version)
{
    QCryptographicHash shasum(QCryptographicHash::Algorithm::Sha1);
    QString result = dataPath + QLatin1Char('-') + version;
    shasum.addData(result.toUtf8());
    return ApplicationId(QLatin1String("qtsingleapp-") + QString::fromLatin1(shasum.result().toHex()));
}

ApplicationId ApplicationId::fromCustomId(const QString& id)
{
    return ApplicationId(QLatin1String("qtsingleapp-") + id);
}

ApplicationId ApplicationId::fromRawString(const QString& id)
{
    return ApplicationId(id);
}

LocalPeer::LocalPeer(QObject * parent, const ApplicationId &appId)
    : QObject(parent), id(appId)
{
    socketName = id.toString();
    server.reset(new QLocalServer());
    QString lockName = QDir(QDir::tempPath()).absolutePath() + QLatin1Char('/') + socketName + QLatin1String("-lockfile");
    lockFile.reset(new LockedFile(lockName));
    lockFile->open(QIODevice::ReadWrite);
}

LocalPeer::~LocalPeer()
{
}

ApplicationId LocalPeer::applicationId() const
{
    return id;
}

bool LocalPeer::isClient()
{
    if (lockFile->isLocked())
        return false;

    if (!lockFile->lock(LockedFile::WriteLock, false))
        return true;

    bool res = server->listen(socketName);
#if defined(Q_OS_UNIX)
    // ### Workaround
    if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
        QFile::remove(QDir::cleanPath(QDir::tempPath())+QLatin1Char('/')+socketName);
        res = server->listen(socketName);
    }
#endif
    if (!res)
        qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qPrintable(server->errorString()));
    QObject::connect(server.get(), SIGNAL(newConnection()), SLOT(receiveConnection()));
    return false;
}


bool LocalPeer::sendMessage(const QByteArray &message, int timeout)
{
    if (!isClient())
        return false;

    QLocalSocket socket;
    bool connOk = false;
    for(int i = 0; i < 2; i++) {
        // Try twice, in case the other instance is just starting up
        socket.connectToServer(socketName);
        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    if (!connOk)
    {
        return false;
    }

    QByteArray uMsg(message);
    QDataStream ds(&socket);

    ds.writeBytes(uMsg.constData(), uMsg.size());
    if(!socket.waitForBytesWritten(timeout))
    {
        return false;
    }

    // wait for 'ack'
    if(!socket.waitForReadyRead(timeout))
    {
        return false;
    }

    // make sure we got 'ack'
    if(!(socket.read(qstrlen(ack)) == ack))
    {
        return false;
    }
    return true;
}


void LocalPeer::receiveConnection()
{
    QLocalSocket* socket = server->nextPendingConnection();
    if (!socket)
    {
        return;
    }

    while (socket->bytesAvailable() < (int)sizeof(quint32))
    {
        socket->waitForReadyRead();
    }
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do
    {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));
    if (got < 0)
    {
        qWarning("QtLocalPeer: Message reception failed %s", socket->errorString().toLatin1().constData());
        delete socket;
        return;
    }
    socket->write(ack, qstrlen(ack));
    socket->waitForBytesWritten(1000);
    socket->waitForDisconnected(1000); // make sure client reads ack
    delete socket;
    emit messageReceived(uMsg); //### (might take a long time to return)
}
