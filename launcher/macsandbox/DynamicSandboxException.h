// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
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

#ifndef LAUNCHER_DYNAMICSANDBOXEXCEPTION_H
#define LAUNCHER_DYNAMICSANDBOXEXCEPTION_H

#include <QList>
#include <QObject>
class QUrl;
class QString;
class SecurityBookmarkFileAccess;

class DynamicSandboxException : public QObject {
    Q_OBJECT

    SecurityBookmarkFileAccess* m_readWriteBookmarkAccess;
    SecurityBookmarkFileAccess* m_readOnlyBookmarkAccess;

    [[nodiscard]] QList<QUrl> bookmarkListSettingToURLs(const QString& settingName) const;

   public:
    explicit DynamicSandboxException(QObject* parent = nullptr);
    ~DynamicSandboxException() override;

    [[nodiscard]] QList<QUrl> readWriteExceptionURLs() const;
    [[nodiscard]] QList<QUrl> readOnlyExceptionURLs() const;

   public slots:
    bool addReadWriteException(const QString& path);
    bool addReadWriteExceptions(const QList<QUrl>& url);
    bool addReadOnlyException(const QString& path);
    bool addReadOnlyExceptions(const QList<QUrl>& url);
    void removeReadWriteException(int index);
    void removeReadOnlyException(int index);
};

#endif  // LAUNCHER_DYNAMICSANDBOXEXCEPTION_H
