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

#ifndef FILEACCESS_H
#define FILEACCESS_H

#include <QtCore/QMap>
#include <QtCore/QSet>
Q_FORWARD_DECLARE_OBJC_CLASS(NSData);
Q_FORWARD_DECLARE_OBJC_CLASS(NSURL);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
Q_FORWARD_DECLARE_OBJC_CLASS(NSAutoreleasePool);
Q_FORWARD_DECLARE_OBJC_CLASS(NSMutableDictionary);
Q_FORWARD_DECLARE_OBJC_CLASS(NSMutableSet);
class QString;
class QByteArray;
class QUrl;

class SecurityBookmarkFileAccess {
    /// The keys are bookmarks and the values are URLs.
    NSMutableDictionary* m_bookmarks;
    /// The keys are paths and the values are bookmarks.
    NSMutableDictionary* m_paths;
    /// Contains URLs that are currently being accessed.
    NSMutableSet* m_activeURLs;

    bool m_readOnly;

    NSURL* securityScopedBookmarkToNSURL(QByteArray& bookmark, bool& isStale);
public:
    /// \param readOnly A boolean indicating whether the bookmark should be read-only.
    SecurityBookmarkFileAccess(bool readOnly = false);
    ~SecurityBookmarkFileAccess();

    /// Get a security scoped bookmark from a URL.
    ///
    /// The URL must be accessible before calling this function. That is, call `startAccessingSecurityScopedResource()` before calling
    /// this function. Note that this is called implicitly if the user selects the directory from a file picker.
    /// \param url The URL to get the security scoped bookmark from.
    /// \return A QByteArray containing the security scoped bookmark.
    QByteArray urlToSecurityScopedBookmark(const QUrl& url);
    /// Get a security scoped bookmark from a path.
    ///
    /// The path must be accessible before calling this function. That is, call `startAccessingSecurityScopedResource()` before calling
    /// this function. Note that this is called implicitly if the user selects the directory from a file picker.
    /// \param path The path to get the security scoped bookmark from.
    /// \return A QByteArray containing the security scoped bookmark.
    QByteArray pathToSecurityScopedBookmark(const QString& path);
    /// Get a QUrl from a security scoped bookmark. If the bookmark is stale, isStale will be set to true and the bookmark will be updated.
    ///
    /// You must check whether the URL is valid before using it.
    /// \param bookmark The security scoped bookmark to get the URL from.
    /// \param isStale A boolean that will be set to true if the bookmark is stale.
    /// \return The URL from the security scoped bookmark.
    QUrl securityScopedBookmarkToURL(QByteArray& bookmark, bool& isStale);

    /// Makes the file or directory at the path pointed to by the bookmark accessible. Unlike `startAccessingSecurityScopedResource()`, this
    /// class ensures that only one "access" is active at a time. Calling this function again after the security-scoped resource has
    /// already been used will do nothing, and a single call to `stopUsingSecurityScopedBookmark()` will release the resource provided that
    /// this is the only `SecurityBookmarkFileAccess` accessing the resource.
    ///
    /// If the bookmark is stale, `isStale` will be set to true and the bookmark will be updated. Stored copies of the bookmark need to be
    /// updated.
    /// \param bookmark The security scoped bookmark to start accessing.
    /// \param isStale A boolean that will be set to true if the bookmark is stale.
    /// \return A boolean indicating whether the bookmark was successfully accessed.
    bool startUsingSecurityScopedBookmark(QByteArray& bookmark, bool& isStale);
    void stopUsingSecurityScopedBookmark(QByteArray& bookmark);

    /// Returns true if access to the `path` is currently being maintained by this object.
    bool isAccessingPath(const QString& path);
};

#endif //FILEACCESS_H
