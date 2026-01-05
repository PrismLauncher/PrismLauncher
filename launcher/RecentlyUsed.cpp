// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 *
 */

#include "RecentlyUsed.h"

#include <QDir>
#include <QLoggingCategory>
#include <QProcess>
#include <utility>
// Os exclusive headers
#if defined(Q_OS_WIN32)

#if defined(Q_OS_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
#include <windows.h>

#include <shlobj_core.h> // SHAddToRecentDocs, SHARDAPPIDINFOLINK, IShellLinkW
#include <shlobj.h>
#include <shlwapi.h>
#include <shlguid.h>     // CLSID_ShellLink, IID_IShellLinkW
#include <objbase.h>     // CoInitialize, CoUninitialize
#include <strsafe.h>
// clang-format on

#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QLockFile>
#include <QMimeDatabase>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>
#include <QXmlStreamWriter>

#elif defined(OS_OS_MAXOS)

#endif

#endif

Q_LOGGING_CATEGORY(RULogCat, "RecentlyUsed");

namespace RecentlyUsed {

#if defined(Q_OS_WIN32)
bool recordRecentlyUsedWin32(const QUrl& url, RecentlyUsedData data, int maxEntries)
{
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        qCDebug(RULogCat) << "Failed to initialize COM:" << Qt::hex << hr;
        return false;
    }

    // create url item
    IShellItem* pItem;
    auto wUrl = url.toString().toStdWString();
    hr = SHCreateItemFromParsingName(wUrl.c_str(), nullptr, IID_PPV_ARGS(&pItem));
    if (FAILED(hr)) {
        qCDebug(RULogCat) << "Failed to creat ShellItem:" << Qt::hex << hr;
        CoUninitialize();
        return false;
    }

    if (!data.title.isEmpty()) {
        auto wTitle = data.title.toStdWString();
        hr = pLink->SetDescription(wTitle.data());
        if (FAILED(hr)) {
            pLink->Release();
            CoUninitialize();
            return false;
        }
    }

    // Prepare SHARDAPPIDINFOLINK structure
    SHARDAPPIDINFOLINK shardLink = {};
    shardLink.psl = pLink;
    shardLink.pszAppID = L"com.example.myapp";

    // Add to recent documents
    SHAddToRecentDocs(SHARD_APPIDINFOLINK, &shardLink);

    pLink->Release();

    CoUninitialize();
    return false;
}
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)

/* This code is based off of and partialy borrowed from the code from the KIO core framework
 * used under a BSD-2-Clause license
 *
 * https://invent.kde.org/frameworks/kio/-/blob/57342c46bf3789cd6f7b07ec33086a24f26223ad/src/core/krecentdocument.cpp
 *
 * SPDX-FileCopyrightText: 2000 Daniel M. Duley <mosfet@kde.org>
 * SPDX-FileCopyrightText: 2021 Martin Tobias Holmedahl Sandsmark
 * SPDX-FileCopyrightText: 2022 Méven Car <meven.car@kdemail.net>
 *
 * Copyright 2022 Daniel M. Duley <mosfet@kde.org> & Martin Tobias Holmedahl Sandsmark & Méven Car <meven.car@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

static const QLatin1String xbelTag("xbel");
static const QLatin1String versionAttribute("version");
static const QLatin1String expectedVersion("1.0");

static const QLatin1String applicationsBookmarkTag("bookmark:applications");
static const QLatin1String applicationBookmarkTag("bookmark:application");
static const QLatin1String bookmarkTag("bookmark");
static const QLatin1String infoTag("info");
static const QLatin1String titleTag("title");
static const QLatin1String descTag("desc");
static const QLatin1String metadataTag("metadata");
static const QLatin1String mimeTypeTag("mime:mime-type");
static const QLatin1String bookmarkGroups("bookmark:groups");
static const QLatin1String bookmarkGroup("bookmark:group");
static const QLatin1String bookmarkIconTag("bookmark:icon");

static const QLatin1String nameAttribute("name");
static const QLatin1String countAttribute("count");
static const QLatin1String modifiedAttribute("modified");
static const QLatin1String visitedAttribute("visited");
static const QLatin1String hrefAttribute("href");
static const QLatin1String typeAttribute("type");
static const QLatin1String addedAttribute("added");
static const QLatin1String execAttribute("exec");
static const QLatin1String ownerAttribute("owner");
static const QLatin1String ownerValue("http://freedesktop.org");
static const QLatin1String typeAttribute("type");

static QString xbelPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/recently-used.xbel");
}

/**
 * @brief removes entries from the xbel bookmarks file untill only maxEntries exist
 *
 * Modified form the origonal in KIO to only remove ourselves from the bookmarks
 * so that `maxEntries` refers to the number of bookmarks that point to us.
 * if a bookmarks has no applications bookmarking it after our removal the bookmark itself is removed
 *
 * @param maxEntries max number of bookmarks that point at us
 * @return if file writing was a success
 */
static bool removeOldestEntries(int& maxEntries)
{
    QFile input(xbelPath());
    if (!input.exists()) {
        return true;
    }

    // Won't help for GTK applications and whatnot, but we can be good citizens ourselves
    QLockFile lockFile(xbelPath() + QLatin1String(".lock"));
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock(100)) {  // give it 100ms
        qCWarning(RULogCat) << "Failed to lock recently used";
        return false;
    }

    if (!input.open(QIODevice::ReadOnly)) {
        qCWarning(RULogCat) << "Failed to open existing recently used" << input.errorString();
        return false;
    }

    QDomDocument document;
    document.setContent(&input);
    input.close();

    auto xbelTags = document.elementsByTagName(xbelTag);
    if (xbelTags.length() != 1) {
        qCWarning(RULogCat) << "Invalid Xbel file, missing xbel element";
        return false;
    }
    auto xbelElement = xbelTags.item(0);
    auto bookmarkList = xbelElement.childNodes();
    if (bookmarkList.length() <= maxEntries) {
        return true;
    }

    // desktopFileName is in QGuiApplication but this should be less restrictive
    QString desktopEntryName = QCoreApplication::instance()->property("desktopFileName").toString();
    if (desktopEntryName.isEmpty()) {
        desktopEntryName = QCoreApplication::applicationName();
    }

    QMultiMap<QDateTime, std::array<QDomNode, 3>> bookmarksByModifiedDate;
    for (int i = 0; i < bookmarkList.length(); ++i) {
        const auto bookmarkNode = bookmarkList.item(i);
        const auto modifiedString = bookmarkNode.attributes().namedItem(modifiedAttribute);
        const auto modifiedTime = QDateTime::fromString(modifiedString.nodeValue(), Qt::ISODate);

        if (const auto infoNode = bookmarkNode.firstChildElement(infoTag); !infoNode.isNull()) {
            if (const auto metadateNode = infoNode.firstChildElement(metadataTag); !metadateNode.isNull()) {
                if (const auto appsBoomarksNode = metadateNode.firstChildElement(applicationsBookmarkTag); !appsBoomarksNode.isNull()) {
                    for (const auto appBookmarkNode : appsBoomarksNode.childNodes()) {
                        if (const auto nameNode = appBookmarkNode.attributes().namedItem(nameAttribute);
                            nameNode.nodeValue() == desktopEntryName) {
                            // this recources is bookmarred by this app. record the nodes.
                            bookmarksByModifiedDate.insert(modifiedTime, { bookmarkNode, appsBoomarksNode, appBookmarkNode });
                            break;
                        }
                    }
                }
            }
        }
    }

    int i = 0;
    // entries are traversed in ascending key order
    for (auto entry = bookmarksByModifiedDate.keyValueBegin(); entry != bookmarksByModifiedDate.keyValueEnd(); ++entry) {
        // only keep the maxEntries last nodes
        if (bookmarksByModifiedDate.size() - i > maxEntries) {
            auto [bookmarkNode, appsBoomarksNode, appBookmarkNode] = entry->second;
            // remove our app fron this recource
            appsBoomarksNode.removeChild(appBookmarkNode);
            // if no other app bookmark this recource, remove it
            if (appsBoomarksNode.childNodes().isEmpty()) {
                xbelElement.removeChild(bookmarkNode);
            }
        }
        ++i;
    }

    if (input.open(QIODevice::WriteOnly) && input.write(document.toByteArray(2)) != -1) {
        return true;
    }
    return false;
}

static bool addToXbel(const QUrl& url, RecentlyUsedData data, int maxEntries)
{
    using namespace Qt::StringLiterals;

    // desktopFileName is in QGuiApplication but this should be less restrictive
    QString desktopEntryName = QCoreApplication::instance()->property("desktopFileName").toString();
    if (desktopEntryName.isEmpty()) {
        desktopEntryName = QCoreApplication::applicationName();
    }

    if (!QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))) {
        qCWarning(RULogCat) << "Could not create GenericDataLocation";
        return false;
    }

    // Won't help for GTK applications and whatnot, but we can be good citizens ourselves
    QLockFile lockFile(xbelPath() + QLatin1String(".lock"));
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock(100)) {  // give it 100ms
        qCWarning(RULogCat) << "Failed to lock recently used";
        return false;
    }

    QByteArray existingContent;
    QFile input(xbelPath());
    if (input.open(QIODevice::ReadOnly)) {
        existingContent = input.readAll();
    } else if (!input.exists()) {  // That it doesn't exist is a very uncommon case
        qCDebug(RULogCat) << input.fileName() << "does not exist, creating new";
    } else {
        qCWarning(RULogCat) << "Failed to open existing recently used" << input.errorString();
        return false;
    }

    QXmlStreamReader xml(existingContent);

    xml.readNextStartElement();
    if (!existingContent.isEmpty()) {
        if (xml.name().isEmpty() || xml.name() != xbelTag || !xml.attributes().hasAttribute(versionAttribute)) {
            qCDebug(RULogCat) << "The recently-used.xbel is not an XBEL file, overwriting.";
        } else if (xml.attributes().value(versionAttribute) != expectedVersion) {
            qCDebug(RULogCat) << "The recently-used.xbel is not an XBEL version 1.0 file but has version: "
                              << xml.attributes().value(versionAttribute) << ", overwriting.";
        }
    }

    QSaveFile outputFile(xbelPath());
    if (!outputFile.open(QIODevice::WriteOnly)) {
        qCWarning(RULogCat) << "Failed to recently-used.xbel for writing:" << outputFile.errorString();
        return false;
    }

    QXmlStreamWriter output(&outputFile);
    output.setAutoFormatting(true);
    output.setAutoFormattingIndent(2);
    output.writeStartDocument();
    output.writeStartElement(xbelTag);

    output.writeAttribute(versionAttribute, expectedVersion);
    output.writeNamespace("http://www.freedesktop.org/standards/desktop-bookmarks"_L1, bookmarkTag);
    output.writeNamespace("http://www.freedesktop.org/standards/shared-mime-info"_L1, "mime"_L1);

    const QString newUrl = QString::fromLatin1(url.toEncoded());

    const QString currentTimestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).chopped(1) + "000Z"_L1;

    auto addApplicationTag = [&output, desktopEntryName, currentTimestamp, url]() {
        output.writeEmptyElement(applicationBookmarkTag);
        output.writeAttribute(nameAttribute, desktopEntryName);

        // FIXME: correctly get a executable command even in appimages if the launcher isn't in $path
        QString exec = QCoreApplication::instance()->applicationName() + QLatin1String(" %u");

        output.writeAttribute(execAttribute, exec);
        output.writeAttribute(modifiedAttribute, currentTimestamp);
        output.writeAttribute(countAttribute, "1"_L1);
    };

    auto addBookmarkIconTag = [&output, &data, desktopEntryName]() {
        output.writeEmptyElement(bookmarkIconTag);
        QString iconMime = "image/png";
        if (!data.iconUrl.isEmpty()) {
            QMimeDatabase mimeDb;
            iconMime = mimeDb.mimeTypeForUrl(data.iconUrl).name();
        }
        output.writeAttribute(typeAttribute, iconMime);
        if (!data.iconUrl.isEmpty()) {
            output.writeAttribute(hrefAttribute, data.iconUrl.toString());
        }
        output.writeAttribute(nameAttribute, desktopEntryName);
    };

    bool foundExistingApp = false;
    bool inRightBookmark = false;
    bool foundMatchingBookmark = false;
    bool firstBookmark = true;
    bool foundTitle = false;
    bool foundDesc = false;
    bool foundIcon = false;
    int nbEntries = 0;

    QMultiMap<QDateTime, std::array<QDomNode, 3>> bookmarksByModifiedDate;

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNext() == QXmlStreamReader::EndElement && xml.name() == xbelTag) {
            break;
        }
        switch (xml.tokenType()) {
            case QXmlStreamReader::StartElement: {
                const QStringView tagName = xml.qualifiedName();
                QXmlStreamAttributes attributes = xml.attributes();

                if (tagName == bookmarkTag) {
                    foundExistingApp = false;
                    firstBookmark = false;

                    const QStringView hrefValue = attributes.value(hrefAttribute);
                    inRightBookmark = hrefValue == newUrl;

                    if (inRightBookmark) {
                        foundMatchingBookmark = true;

                        QXmlStreamAttributes newAttributes;
                        for (const QXmlStreamAttribute& old : attributes) {
                            if (old.name() == modifiedAttribute) {
                                continue;
                            }
                            if (old.name() == visitedAttribute) {
                                continue;
                            }
                            newAttributes.append(old);
                        }
                        newAttributes.append(modifiedAttribute, currentTimestamp);
                        newAttributes.append(visitedAttribute, currentTimestamp);
                        attributes = newAttributes;
                    }
                }

                if (tagName == applicationBookmarkTag && attributes.value(nameAttribute) == desktopEntryName) {
                    nbEntries += 1;
                }

                else if (inRightBookmark && tagName == applicationBookmarkTag && attributes.value(nameAttribute) == desktopEntryName) {
                    // case found right bookmark and same application
                    const int count = attributes.value(countAttribute).toInt();

                    QXmlStreamAttributes newAttributes;
                    for (const QXmlStreamAttribute& old : std::as_const(attributes)) {
                        if (old.name() == countAttribute) {
                            continue;
                        }
                        if (old.name() == modifiedAttribute) {
                            continue;
                        }
                        newAttributes.append(old);
                    }
                    newAttributes.append(modifiedAttribute, currentTimestamp);
                    newAttributes.append(countAttribute, QString::number(count + 1));
                    attributes = newAttributes;

                    foundExistingApp = true;
                } else if (inRightBookmark && tagName == titleTag) {
                    foundTitle = true;
                    QString _title = xml.readElementText(QXmlStreamReader::SkipChildElements);
                    output.writeCharacters(data.title);
                } else if (inRightBookmark && tagName == descTag) {
                    foundDesc = true;
                    QString _desc = xml.readElementText(QXmlStreamReader::SkipChildElements);
                    output.writeCharacters(data.desc);
                } else if (inRightBookmark && tagName == bookmarkIconTag) {
                    foundIcon = true;
                    QXmlStreamAttributes newAttributes;
                    QString iconMime = "image/png";
                    if (!data.iconUrl.isEmpty()) {
                        QMimeDatabase mimeDb;
                        iconMime = mimeDb.mimeTypeForUrl(data.iconUrl).name();
                    }
                    newAttributes.append(typeAttribute, iconMime);
                    if (!data.iconUrl.isEmpty()) {
                        newAttributes.append(hrefAttribute, data.iconUrl.toString());
                    }
                    newAttributes.append(nameAttribute, desktopEntryName);
                    attributes = newAttributes;
                }

                output.writeStartElement(tagName.toString());
                output.writeAttributes(attributes);
                break;
            }
            case QXmlStreamReader::EndElement: {
                const QStringView tagName = xml.qualifiedName();
                if (tagName == applicationsBookmarkTag && inRightBookmark && !foundExistingApp) {
                    // add an application to the applications already known for the bookmark
                    addApplicationTag();
                } else if (tagName == bookmarkTag && inRightBookmark && !foundTitle && !data.title.isEmpty()) {
                    output.writeTextElement(titleTag, data.title);
                } else if (tagName == bookmarkTag && inRightBookmark && !foundDesc && !data.desc.isEmpty()) {
                    output.writeTextElement(descTag, data.desc);
                } else if (tagName == metadataTag && inRightBookmark && !foundIcon) {
                    addBookmarkIconTag();
                }
                output.writeEndElement();
                break;
            }
            case QXmlStreamReader::Characters:
                if (xml.isCDATA()) {
                    output.writeCDATA(xml.text().toString());
                } else {
                    output.writeCharacters(xml.text().toString());
                }
                break;
            case QXmlStreamReader::Comment:
                output.writeComment(xml.text().toString());
                break;
            case QXmlStreamReader::EndDocument:
                qCWarning(RULogCat) << "Malformed, got end document before end of xbel" << xml.tokenString() << url;
                return false;
            default:
                qCWarning(RULogCat) << "unhandled token" << xml.tokenString() << url;
                break;
        }
    }

    if (!foundMatchingBookmark) {
        // must create new bookmark tag
        if (firstBookmark) {
            output.writeCharacters("\n"_L1);
        }
        output.writeCharacters("  "_L1);
        output.writeStartElement(bookmarkTag);

        output.writeAttribute(hrefAttribute, newUrl);
        output.writeAttribute(addedAttribute, currentTimestamp);
        output.writeAttribute(modifiedAttribute, currentTimestamp);
        output.writeAttribute(visitedAttribute, currentTimestamp);

        {
            output.writeTextElement(titleTag, data.title);
            output.writeTextElement(descTag, data.desc);
        }

        {
            const QString urlMime = "x-scheme-handler/prismlauncher";

            output.writeStartElement(infoTag);
            output.writeStartElement(metadataTag);
            output.writeAttribute(ownerAttribute, ownerValue);

            output.writeEmptyElement(mimeTypeTag);
            output.writeAttribute(typeAttribute, urlMime);

            {
                output.writeStartElement(bookmarkGroups);
                output.writeTextElement(bookmarkGroup, "Multimedia");
                // bookmarkGroups
                output.writeEndElement();
            }

            {
                output.writeStartElement(applicationsBookmarkTag);
                addApplicationTag();
                // end applicationsBookmarkTag
                output.writeEndElement();
            }

            addBookmarkIconTag();

            // end metadataTag
            output.writeEndElement();
            // end infoTag
            output.writeEndElement();
        }

        // end bookmarkTag
        output.writeEndElement();
    }

    // end xbelTag
    output.writeEndElement();

    // end document
    output.writeEndDocument();

    if (outputFile.commit()) {
        lockFile.unlock();
        // tolerate 4 more entries than threshold to limit overhead of cleaning old data
        return nbEntries - maxEntries > 4 || removeOldestEntries(maxEntries);
    }
    return false;
}

/**
 * @brief record a url as recently used by this app
 * This is done by editing the recently-used.xbel file acording to the
 * freedesktop.org desktop bookmark spec
 * https://www.freedesktop.org/wiki/Specifications/desktop-bookmark-spec
 */
bool recordRecentlyUsedFreeDesktop(const QUrl& url, RecentlyUsedData data, int maxEntries)
{
    return addToXbel(url, data, maxEntries);
}

#endif

#if defined(OS_OS_MACOS)
bool recordRecentlyUsedMacos(const QUrl& url)
{
    return false;
}
#endif

bool recordRecentlyUsed(const QUrl& url, RecentlyUsedData data)
{
    int maxEntries = 6;  // FIXME: Fetch this from a setting?
#if defined Q_OS_WIN32
    return recordRecentlyUsedWin32(url, data, maxEntries);
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    return recordRecentlyUsedFreeDesktop(url, data, maxEntries);
#elif defined(OS_OS_MAXOS)
    return recordRecentlyUsedMacos(url, data, maxEntries);
#else
    qDebug() << "recording recently used resources is not supported on this os";
    return false;
#endif
}
}  // namespace RecentlyUsed
