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

#include "MinecraftAccount.h"

#include <QObject>
#include <QVariant>
#include <QAbstractListModel>
#include <QSharedPointer>

/*!
 * List of available Mojang accounts.
 * This should be loaded in the background by MultiMC on startup.
 */
class AccountList : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ModelRoles
    {
        PointerRole = 0x34B1CB48
    };

    enum VListColumns
    {
        // TODO: Add icon column.
        NameColumn = 0,
        ProfileNameColumn,
        MigrationColumn,
        TypeColumn,

        NUM_COLUMNS
    };

    explicit AccountList(QObject *parent = 0);

    const MinecraftAccountPtr at(int i) const;
    int count() const;

    //////// List Model Functions ////////
    QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void addAccount(const MinecraftAccountPtr account);
    void removeAccount(QModelIndex index);
    int findAccountByProfileId(const QString &profileId) const;
    MinecraftAccountPtr getAccountByProfileName(const QString &profileName) const;

    /*!
     * Sets the path to load/save the list file from/to.
     * If autosave is true, this list will automatically save to the given path whenever it changes.
     * THIS FUNCTION DOES NOT LOAD THE LIST. If you set autosave, be sure to call loadList() immediately
     * after calling this function to ensure an autosaved change doesn't overwrite the list you intended
     * to load.
     */
    void setListFilePath(QString path, bool autosave = false);

    bool loadList();
    bool loadV2(QJsonObject &root);
    bool loadV3(QJsonObject &root);
    bool saveList();

    MinecraftAccountPtr activeAccount() const;
    void setActiveAccount(const QString &profileId);
    bool anyAccountIsValid();

signals:
    void listChanged();
    void activeAccountChanged();

public slots:
    /**
     * This is called when one of the accounts changes and the list needs to be updated
     */
    void accountChanged();

protected:
    /*!
     * Called whenever the list changes.
     * This emits the listChanged() signal and autosaves the list (if autosave is enabled).
     */
    void onListChanged();

    /*!
     * Called whenever the active account changes.
     * Emits the activeAccountChanged() signal and autosaves the list if enabled.
     */
    void onActiveChanged();

    QList<MinecraftAccountPtr> m_accounts;

    MinecraftAccountPtr m_activeAccount;

    //! Path to the account list file. Empty string if there isn't one.
    QString m_listFilePath;

    /*!
     * If true, the account list will automatically save to the account list path when it changes.
     * Ignored if m_listFilePath is blank.
     */
    bool m_autosave = false;
};
