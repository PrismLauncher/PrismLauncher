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
#include <QList>

#include <net/NetJob.h>

#include "NewsEntry.h"

class NewsChecker : public QObject
{
    Q_OBJECT
public:
    /*!
     * Constructs a news reader to read from the given RSS feed URL.
     */
    NewsChecker(shared_qobject_ptr<QNetworkAccessManager> network, const QString& feedUrl);

    /*!
     * Returns the error message for the last time the news was loaded.
     * Empty string if the last load was successful.
     */
    QString getLastLoadErrorMsg() const;

    /*!
     * Returns true if the news has been loaded successfully.
     */
    bool isNewsLoaded() const;

    //! True if the news is currently loading. If true, reloadNews() will do nothing.
    bool isLoadingNews() const;

    /*!
     * Returns a list of news entries.
     */
    QList<NewsEntryPtr> getNewsEntries() const;

    /*!
     * Reloads the news from the website's RSS feed.
     * If the news is already loading, this does nothing.
     */
    void Q_SLOT reloadNews();

signals:
    /*!
     * Signal fired after the news has finished loading.
     */
    void newsLoaded();

    /*!
     * Signal fired after the news fails to load.
     */
    void newsLoadingFailed(QString errorMsg);

protected slots:
    void rssDownloadFinished();
    void rssDownloadFailed(QString reason);

protected: /* data */
    //! The URL for the RSS feed to fetch.
    QString m_feedUrl;

    //! List of news entries.
    QList<NewsEntryPtr> m_newsEntries;

    //! The network job to use to load the news.
    NetJob::Ptr m_newsNetJob;

    //! True if news has been loaded.
    bool m_loadedNews;

    std::shared_ptr<QByteArray> newsData = std::make_shared<QByteArray>();

    /*!
     * Gets the error message that was given last time the news was loaded.
     * If the last news load succeeded, this will be an empty string.
     */
    QString m_lastLoadError;

    shared_qobject_ptr<QNetworkAccessManager> m_network;

protected slots:
    /// Emits newsLoaded() and sets m_lastLoadError to empty string.
    void succeed();

    /// Emits newsLoadingFailed() and sets m_lastLoadError to the given message.
    void fail(const QString& errorMsg);
};

