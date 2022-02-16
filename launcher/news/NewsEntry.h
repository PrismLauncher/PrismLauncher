/* Copyright 2013-2021 MultiMC Contributors
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

#pragma once

#include <QObject>
#include <QString>
#include <QDomElement>
#include <memory>

class NewsEntry : public QObject
{
    Q_OBJECT

public:
    /*!
     * Constructs an empty news entry.
     */
    explicit NewsEntry(QObject* parent=0);

    /*!
     * Constructs a new news entry.
     * Note that content may contain HTML.
     */
    NewsEntry(const QString& title, const QString& content, const QString& link, QObject* parent=0);

    /*!
     * Attempts to load information from the given XML element into the given news entry pointer.
     * If this fails, the function will return false and store an error message in the errorMsg pointer.
     */
    static bool fromXmlElement(const QDomElement& element, NewsEntry* entry, QString* errorMsg=0);


    //! The post title.
    QString title;

    //! The post's content. May contain HTML.
    QString content;

    //! URL to the post.
    QString link;
};

typedef std::shared_ptr<NewsEntry> NewsEntryPtr;

