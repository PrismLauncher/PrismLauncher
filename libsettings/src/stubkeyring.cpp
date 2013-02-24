/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stubkeyring.h"

#include <QStringList>

// Scrambling
// this is NOT SAFE, but it's not plain either.
int scrambler = 0x9586309;

QString scramble(QString in_)
{
    QByteArray in = in_.toUtf8();
    QByteArray out;
    for (int i = 0; i<in.length(); i++)
        out.append(in.at(i) ^ scrambler);
    return QString::fromUtf8(out);
}

inline QString base64(QString in)
{
    return QString(in.toUtf8().toBase64());
}
inline QString unbase64(QString in)
{
    return QString::fromUtf8(QByteArray::fromBase64(in.toLatin1()));
}

inline QString scramble64(QString in)
{
    return base64(scramble(in));
}
inline QString unscramble64(QString in)
{
    return scramble(unbase64(in));
}

// StubKeyring implementation
inline QString generateKey(QString service, QString username)
{
    return QString("%1/%2").arg(base64(service)).arg(scramble64(username));
}

bool StubKeyring::storePassword(QString service, QString username, QString password)
{
    m_settings.setValue(generateKey(service, username), scramble64(password));
    return true;
}

QString StubKeyring::getPassword(QString service, QString username)
{
    QString key = generateKey(service, username);
    if (!m_settings.contains(key))
        return QString();
    return unscramble64(m_settings.value(key).toString());
}

inline bool StubKeyring::hasPassword(QString service, QString username)
{
    return m_settings.contains(generateKey(service, username));
}

QStringList StubKeyring::getStoredAccounts(QString service)
{
    service = base64(service).append('/');
    QStringList out;
    QStringList in(m_settings.allKeys());
    QStringListIterator it(in);
    while(it.hasNext())
    {
        QString c = it.next();
        if (c.startsWith(service))
            out << unscramble64(c.mid(service.length()));
    }
    return out;
}

StubKeyring::StubKeyring() :
    m_settings(QSettings::UserScope, "Orochimarufan", "Keyring")
{
}
