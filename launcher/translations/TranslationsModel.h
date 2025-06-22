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

#include <QAbstractListModel>
#include <memory>
#include <optional>

struct Language;

class TranslationsModel : public QAbstractListModel {
    Q_OBJECT
   public:
    explicit TranslationsModel(QString path, QObject* parent = 0);
    virtual ~TranslationsModel();

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;

    bool selectLanguage(QString key);
    void updateLanguage(QString key);
    QModelIndex selectedIndex();
    QString selectedLanguage();

    void downloadIndex();
    void setUseSystemLocale(bool useSystemLocale);

   private:
    QList<Language>::Iterator findLanguage(const QString& key);
    std::optional<Language> findLanguageAsOptional(const QString& key);
    void reloadLocalFiles();
    void downloadTranslation(QString key);
    void downloadNext();

    // hide copy constructor
    TranslationsModel(const TranslationsModel&) = delete;
    // hide assign op
    TranslationsModel& operator=(const TranslationsModel&) = delete;

   private slots:
    void indexReceived();
    void indexFailed(QString reason);
    void dlFailed(QString reason);
    void dlGood();
    void translationDirChanged(const QString& path);

   private: /* data */
    struct Private;
    std::unique_ptr<Private> d;
};
