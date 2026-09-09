// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDir>
#include <QString>
#include <QtGlobal>
#include <QUuid>

inline QString launchNativePath(const QString& nativeRoot, quint64 sessionId)
{
    static const auto processKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QDir(nativeRoot).absoluteFilePath(QString("session-%1-%2").arg(processKey).arg(sessionId));
}
