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

#include "DynamicSandboxException.h"
#include "Application.h"
#include "SecurityBookmarkFileAccess.h"

DynamicSandboxException::DynamicSandboxException(QObject* parent) : QObject(parent)
{
    m_readWriteBookmarkAccess = new SecurityBookmarkFileAccess(false);
    m_readOnlyBookmarkAccess = new SecurityBookmarkFileAccess(true);

    QList<QVariant> readWriteBookmarks = APPLICATION->settings()->get("ReadWriteDynamicSandboxExceptions").toList();
    QList<QVariant> readOnlyBookmarks = APPLICATION->settings()->get("ReadOnlyDynamicSandboxExceptions").toList();

    for (QVariant& path : readWriteBookmarks) {
        bool isStale = false;
        QByteArray bookmark = path.toByteArray();
        m_readWriteBookmarkAccess->startUsingSecurityScopedBookmark(bookmark, isStale);
        if (isStale) {
            path = QVariant::fromValue(bookmark);
        }
    }
    for (QVariant& path : readOnlyBookmarks) {
        bool isStale = false;
        QByteArray bookmark = path.toByteArray();
        m_readOnlyBookmarkAccess->startUsingSecurityScopedBookmark(bookmark, isStale);
        if (isStale) {
            path = QVariant::fromValue(bookmark);
        }
    }
    APPLICATION->settings()->set("ReadWriteDynamicSandboxExceptions", readWriteBookmarks);
    APPLICATION->settings()->set("ReadOnlyDynamicSandboxExceptions", readOnlyBookmarks);
}

DynamicSandboxException::~DynamicSandboxException() {
    delete m_readWriteBookmarkAccess;
    delete m_readOnlyBookmarkAccess;
}

QList<QUrl> DynamicSandboxException::bookmarkListSettingToURLs(const QString& settingName) const
{
    QList<QVariant> bookmarkList = APPLICATION->settings()->get(settingName).toList();
    QList<QUrl> urls;
    SecurityBookmarkFileAccess* access = settingName == "ReadWriteDynamicSandboxExceptions" ? m_readWriteBookmarkAccess : m_readOnlyBookmarkAccess;
    for (auto & item : bookmarkList) {
        bool isStale = false;
        QByteArray bookmark = item.toByteArray();
        QUrl url = access->securityScopedBookmarkToURL(bookmark, isStale);
        if (isStale) {
            item = bookmark;
        }
        urls.push_back(url);
    }
    APPLICATION->settings()->set(settingName, bookmarkList);
    return urls;
}

QList<QUrl> DynamicSandboxException::readWriteExceptionURLs() const
{
    return bookmarkListSettingToURLs("ReadWriteDynamicSandboxExceptions");
}

QList<QUrl> DynamicSandboxException::readOnlyExceptionURLs() const
{
    return bookmarkListSettingToURLs("ReadOnlyDynamicSandboxExceptions");
}

bool DynamicSandboxException::addReadWriteException(const QString& path) {
    if (m_readWriteBookmarkAccess->isAccessingPath(path)) {
        return false;
    }
    QByteArray bookmark = m_readWriteBookmarkAccess->pathToSecurityScopedBookmark(path);
    QList<QVariant> readWriteBookmarks = APPLICATION->settings()->get("ReadWriteDynamicSandboxExceptions").toList();
    readWriteBookmarks.push_back(bookmark);
    APPLICATION->settings()->set("ReadWriteDynamicSandboxExceptions", readWriteBookmarks);
    return true;
}

bool DynamicSandboxException::addReadOnlyException(const QString& path) {
    if (m_readOnlyBookmarkAccess->isAccessingPath(path)) {
        return false;
    }
    QByteArray bookmark = m_readOnlyBookmarkAccess->pathToSecurityScopedBookmark(path);
    QList<QVariant> readOnlyBookmarks = APPLICATION->settings()->get("ReadOnlyDynamicSandboxExceptions").toList();
    readOnlyBookmarks.push_back(bookmark);
    APPLICATION->settings()->set("ReadOnlyDynamicSandboxExceptions", readOnlyBookmarks);
    return true;
}

void DynamicSandboxException::removeReadWriteException(int index) {
    QList<QVariant> readWriteBookmarks = APPLICATION->settings()->get("ReadWriteDynamicSandboxExceptions").toList();
    QByteArray bookmark = readWriteBookmarks.at(index).toByteArray();
    m_readWriteBookmarkAccess->stopUsingSecurityScopedBookmark(bookmark);
    readWriteBookmarks.removeAt(index);
    APPLICATION->settings()->set("ReadWriteDynamicSandboxExceptions", readWriteBookmarks);
}

void DynamicSandboxException::removeReadOnlyException(int index) {
    QList<QVariant> readOnlyBookmarks = APPLICATION->settings()->get("ReadOnlyDynamicSandboxExceptions").toList();
    QByteArray bookmark = readOnlyBookmarks.at(index).toByteArray();
    m_readOnlyBookmarkAccess->stopUsingSecurityScopedBookmark(bookmark);
    readOnlyBookmarks.removeAt(index);
    APPLICATION->settings()->set("ReadOnlyDynamicSandboxExceptions", readOnlyBookmarks);
}

bool DynamicSandboxException::addReadWriteExceptions(const QList<QUrl>& url)
{
    bool success = true;
    for (const QUrl& u : url) {
        if (!u.isLocalFile()) {
            continue;
        }
        QString path = u.toLocalFile();
        success = success && addReadWriteException(path);
    }
    return success;
}

bool DynamicSandboxException::addReadOnlyExceptions(const QList<QUrl>& url)
{
    bool success = true;
    for (const QUrl& u : url) {
        if (!u.isLocalFile()) {
            continue;
        }
        QString path = u.toLocalFile();
        success = success && addReadOnlyException(path);
    }
    return success;
}
