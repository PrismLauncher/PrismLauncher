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

#include "SecurityBookmarkFileAccess.h"

#include <Foundation/Foundation.h>
#include <QByteArray>
#include <QUrl>

QByteArray SecurityBookmarkFileAccess::urlToSecurityScopedBookmark(const QUrl& url)
{
    if (!url.isLocalFile())
        return {};

    NSError* error = nil;
    NSURL* nsurl = [url.toNSURL() absoluteURL];
    NSData* bookmark;
    if ([m_paths objectForKey:[nsurl path]]) {
        bookmark = m_paths[[nsurl path]];
    } else {
        bookmark = [nsurl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope |
                                                  (m_readOnly ? NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess : 0)
                   includingResourceValuesForKeys:nil
                                    relativeToURL:nil
                                            error:&error];
    }
    if (error) {
        return {};
    }

    // remove/reapply access to ensure that write access is immediately cut off for read-only bookmarks
    // sometimes you need to call this twice to actually stop access (extra calls aren't harmful)
    [nsurl stopAccessingSecurityScopedResource];
    [nsurl stopAccessingSecurityScopedResource];
    nsurl = [NSURL URLByResolvingBookmarkData:bookmark
                                      options:NSURLBookmarkResolutionWithSecurityScope |
                                              (m_readOnly ? NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess : 0)
                                relativeToURL:nil
                          bookmarkDataIsStale:nil
                                        error:&error];
    m_paths[[nsurl path]] = bookmark;
    m_bookmarks[bookmark] = nsurl;

    QByteArray qBookmark = QByteArray::fromNSData(bookmark);
    bool isStale = false;
    startUsingSecurityScopedBookmark(qBookmark, isStale);

    return qBookmark;
}

SecurityBookmarkFileAccess::SecurityBookmarkFileAccess(bool readOnly) : m_readOnly(readOnly)
{
    m_bookmarks = [NSMutableDictionary new];
    m_paths = [NSMutableDictionary new];
    m_activeURLs = [NSMutableSet new];
}

SecurityBookmarkFileAccess::~SecurityBookmarkFileAccess()
{
    for (NSURL* url : m_activeURLs) {
        [url stopAccessingSecurityScopedResource];
    }
}

QByteArray SecurityBookmarkFileAccess::pathToSecurityScopedBookmark(const QString& path)
{
    return urlToSecurityScopedBookmark(QUrl::fromLocalFile(path));
}

NSURL* SecurityBookmarkFileAccess::securityScopedBookmarkToNSURL(QByteArray& bookmark, bool& isStale)
{
    NSError* error = nil;
    BOOL localStale = NO;
    NSURL* nsurl = [NSURL URLByResolvingBookmarkData:bookmark.toNSData()
                                             options:NSURLBookmarkResolutionWithSecurityScope |
                                                     (m_readOnly ? NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess : 0)
                                       relativeToURL:nil
                                 bookmarkDataIsStale:&localStale
                                               error:&error];
    if (error) {
        return nil;
    }
    isStale = localStale;
    if (isStale) {
        NSData* nsBookmark = [nsurl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope |
                                                            (m_readOnly ? NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess : 0)
                             includingResourceValuesForKeys:nil
                                              relativeToURL:nil
                                                      error:&error];
        if (error) {
            return nil;
        }
        bookmark = QByteArray::fromNSData(nsBookmark);
    }

    NSData* nsBookmark = bookmark.toNSData();
    m_paths[[nsurl path]] = nsBookmark;
    m_bookmarks[nsBookmark] = nsurl;

    return nsurl;
}

QUrl SecurityBookmarkFileAccess::securityScopedBookmarkToURL(QByteArray& bookmark, bool& isStale)
{
    if (bookmark.isEmpty())
        return {};

    NSURL* url = securityScopedBookmarkToNSURL(bookmark, isStale);
    if (!url)
        return {};

    return QUrl::fromNSURL(url);
}

bool SecurityBookmarkFileAccess::startUsingSecurityScopedBookmark(QByteArray& bookmark, bool& isStale)
{
    NSURL* url = [m_bookmarks objectForKey:bookmark.toNSData()] ? m_bookmarks[bookmark.toNSData()]
                                                                : securityScopedBookmarkToNSURL(bookmark, isStale);
    if ([m_activeURLs containsObject:url])
        return false;

    [url stopAccessingSecurityScopedResource];
    if ([url startAccessingSecurityScopedResource]) {
        [m_activeURLs addObject:url];
        return true;
    }
    return false;
}

void SecurityBookmarkFileAccess::stopUsingSecurityScopedBookmark(QByteArray& bookmark)
{
    if (![m_bookmarks objectForKey:bookmark.toNSData()])
        return;
    NSURL* url = m_bookmarks[bookmark.toNSData()];

    if ([m_activeURLs containsObject:url]) {
        [url stopAccessingSecurityScopedResource];
        [url stopAccessingSecurityScopedResource];

        [m_activeURLs removeObject:url];
        [m_paths removeObjectForKey:[url path]];
        [m_bookmarks removeObjectForKey:bookmark.toNSData()];
    }
}
bool SecurityBookmarkFileAccess::isAccessingPath(const QString& path)
{
    NSData* bookmark = [m_paths objectForKey:path.toNSString()];
    if (!bookmark && path.endsWith('/')) {
        bookmark = [m_paths objectForKey:path.left(path.length() - 1).toNSString()];
    }
    if (!bookmark) {
        return false;
    }
    NSURL* url = [m_bookmarks objectForKey:bookmark];
    return [m_activeURLs containsObject:url];
}
