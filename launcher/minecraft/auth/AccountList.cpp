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

#include "AccountList.h"
#include "AccountData.h"

#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>

#include <QDebug>

#include <FileSystem.h>
#include <QSaveFile>

enum AccountListVersion {
    MojangOnly = 2,
    MojangMSA = 3
};

AccountList::AccountList(QObject *parent) : QAbstractListModel(parent) { }

int AccountList::findAccountByProfileId(const QString& profileId) const {
    for (int i = 0; i < count(); i++) {
        MinecraftAccountPtr account = at(i);
        if (account->profileId() == profileId) {
            return i;
        }
    }
    return -1;
}

const MinecraftAccountPtr AccountList::at(int i) const
{
    return MinecraftAccountPtr(m_accounts.at(i));
}

void AccountList::addAccount(const MinecraftAccountPtr account)
{
    // We only ever want accounts with valid profiles.
    // Keeping profile-less accounts is pointless and serves no purpose.
    auto profileId = account->profileId();
    if(!profileId.size()) {
        return;
    }

    // override/replace existing account with the same profileId
    auto existingAccount = findAccountByProfileId(profileId);
    if(existingAccount != -1) {
        m_accounts[existingAccount] = account;
        emit dataChanged(index(existingAccount), index(existingAccount, columnCount(QModelIndex()) - 1));
        onListChanged();
        return;
    }

    // if we don't have this porfileId yet, add the account to the end
    int row = m_accounts.count();
    beginInsertRows(QModelIndex(), row, row);
    connect(account.get(), SIGNAL(changed()), SLOT(accountChanged()));
    m_accounts.append(account);
    endInsertRows();
    onListChanged();
}

void AccountList::removeAccount(QModelIndex index)
{
    int row = index.row();
    if(index.isValid() && row >= 0 && row < m_accounts.size())
    {
        auto & account = m_accounts[row];
        if(account == m_activeAccount)
        {
            m_activeAccount = nullptr;
            onActiveChanged();
        }
        beginRemoveRows(QModelIndex(), row, row);
        m_accounts.removeAt(index.row());
        endRemoveRows();
        onListChanged();
    }
}

MinecraftAccountPtr AccountList::activeAccount() const
{
    return m_activeAccount;
}

void AccountList::setActiveAccount(const QString &profileId)
{
    if (profileId.isEmpty() && m_activeAccount)
    {
        int idx = 0;
        auto prevActiveAcc = m_activeAccount;
        m_activeAccount = nullptr;
        for (MinecraftAccountPtr account : m_accounts)
        {
            if (account == prevActiveAcc)
            {
                emit dataChanged(index(idx), index(idx));
            }
            idx ++;
        }
        onActiveChanged();
    }
    else
    {
        auto currentActiveAccount = m_activeAccount;
        int currentActiveAccountIdx = -1;
        auto newActiveAccount = m_activeAccount;
        int newActiveAccountIdx = -1;
        int idx = 0;
        for (MinecraftAccountPtr account : m_accounts)
        {
            if (account->profileId() == profileId)
            {
                newActiveAccount = account;
                newActiveAccountIdx = idx;
            }
            if(currentActiveAccount == account)
            {
                currentActiveAccountIdx = idx;
            }
            idx++;
        }
        if(currentActiveAccount != newActiveAccount)
        {
            emit dataChanged(index(currentActiveAccountIdx), index(currentActiveAccountIdx));
            emit dataChanged(index(newActiveAccountIdx), index(newActiveAccountIdx));
            m_activeAccount = newActiveAccount;
            onActiveChanged();
        }
    }
}

void AccountList::accountChanged()
{
    // the list changed. there is no doubt.
    onListChanged();
}

void AccountList::onListChanged()
{
    if (m_autosave)
        // TODO: Alert the user if this fails.
        saveList();

    emit listChanged();
}

void AccountList::onActiveChanged()
{
    if (m_autosave)
        saveList();

    emit activeAccountChanged();
}

int AccountList::count() const
{
    return m_accounts.count();
}

QVariant AccountList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() > count())
        return QVariant();

    MinecraftAccountPtr account = at(index.row());

    switch (role)
    {
        case Qt::DisplayRole:
            switch (index.column())
            {
            case NameColumn:
                return account->accountDisplayString();

            case TypeColumn: {
                auto typeStr = account->typeString();
                typeStr[0] = typeStr[0].toUpper();
                return typeStr;
            }

            case ProfileNameColumn: {
                return account->profileName();
            }

            default:
                return QVariant();
            }

        case Qt::ToolTipRole:
            return account->accountDisplayString();

        case PointerRole:
            return qVariantFromValue(account);

        case Qt::CheckStateRole:
            switch (index.column())
            {
                case NameColumn:
                    return account == m_activeAccount ? Qt::Checked : Qt::Unchecked;
            }

        default:
            return QVariant();
    }
}

QVariant AccountList::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        switch (section)
        {
        case NameColumn:
            return tr("Account");
        case TypeColumn:
            return tr("Type");
        case ProfileNameColumn:
            return tr("Profile");
        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        switch (section)
        {
        case NameColumn:
            return tr("User name of the account.");
        case TypeColumn:
            return tr("Type of the account - Mojang or MSA.");
        case ProfileNameColumn:
            return tr("Name of the Minecraft profile associated with the account.");
        default:
            return QVariant();
        }

    default:
        return QVariant();
    }
}

int AccountList::rowCount(const QModelIndex &) const
{
    // Return count
    return count();
}

int AccountList::columnCount(const QModelIndex &) const
{
    return NUM_COLUMNS;
}

Qt::ItemFlags AccountList::flags(const QModelIndex &index) const
{
    if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool AccountList::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
    {
        return false;
    }

    if(role == Qt::CheckStateRole)
    {
        if(value == Qt::Checked)
        {
            MinecraftAccountPtr account = at(index.row());
            setActiveAccount(account->profileId());
        }
    }

    emit dataChanged(index, index);
    return true;
}

bool AccountList::loadList()
{
    if (m_listFilePath.isEmpty())
    {
        qCritical() << "Can't load Mojang account list. No file path given and no default set.";
        return false;
    }

    QFile file(m_listFilePath);

    // Try to open the file and fail if we can't.
    // TODO: We should probably report this error to the user.
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << QString("Failed to read the account list file (%1).").arg(m_listFilePath).toUtf8();
        return false;
    }

    // Read the file and close it.
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    // Fail if the JSON is invalid.
    if (parseError.error != QJsonParseError::NoError)
    {
        qCritical() << QString("Failed to parse account list file: %1 at offset %2")
                            .arg(parseError.errorString(), QString::number(parseError.offset))
                            .toUtf8();
        return false;
    }

    // Make sure the root is an object.
    if (!jsonDoc.isObject())
    {
        qCritical() << "Invalid account list JSON: Root should be an array.";
        return false;
    }

    QJsonObject root = jsonDoc.object();

    // Make sure the format version matches.
    auto listVersion = root.value("formatVersion").toVariant().toInt();
    switch(listVersion) {
        case AccountListVersion::MojangOnly: {
            return loadV2(root);
        }
        break;
        case AccountListVersion::MojangMSA: {
            return loadV3(root);
        }
        break;
        default: {
            QString newName = "accounts-old.json";
            qWarning() << "Unknown format version when loading account list. Existing one will be renamed to" << newName;
            // Attempt to rename the old version.
            file.rename(newName);
            return false;
        }
    }
}

bool AccountList::loadV2(QJsonObject& root) {
    beginResetModel();
    auto activeUserName = root.value("activeAccount").toString("");
    QJsonArray accounts = root.value("accounts").toArray();
    for (QJsonValue accountVal : accounts)
    {
        QJsonObject accountObj = accountVal.toObject();
        MinecraftAccountPtr account = MinecraftAccount::loadFromJsonV2(accountObj);
        if (account.get() != nullptr)
        {
            auto profileId = account->profileId();
            if(!profileId.size()) {
                continue;
            }
            if(findAccountByProfileId(profileId) != -1) {
                continue;
            }
            connect(account.get(), &MinecraftAccount::changed, this, &AccountList::accountChanged);
            m_accounts.append(account);
            if (activeUserName.size() && account->mojangUserName() == activeUserName) {
                m_activeAccount = account;
            }
        }
        else
        {
            qWarning() << "Failed to load an account.";
        }
    }
    endResetModel();
    return true;
}

bool AccountList::loadV3(QJsonObject& root) {
    beginResetModel();
    QJsonArray accounts = root.value("accounts").toArray();
    for (QJsonValue accountVal : accounts)
    {
        QJsonObject accountObj = accountVal.toObject();
        MinecraftAccountPtr account = MinecraftAccount::loadFromJsonV3(accountObj);
        if (account.get() != nullptr)
        {
            auto profileId = account->profileId();
            if(!profileId.size()) {
                continue;
            }
            if(findAccountByProfileId(profileId) != -1) {
                continue;
            }
            connect(account.get(), &MinecraftAccount::changed, this, &AccountList::accountChanged);
            m_accounts.append(account);
            if(accountObj.value("active").toBool(false)) {
                m_activeAccount = account;
            }
        }
        else
        {
            qWarning() << "Failed to load an account.";
        }
    }
    endResetModel();
    return true;
}


bool AccountList::saveList()
{
    if (m_listFilePath.isEmpty())
    {
        qCritical() << "Can't save Mojang account list. No file path given and no default set.";
        return false;
    }

    // make sure the parent folder exists
    if(!FS::ensureFilePathExists(m_listFilePath))
        return false;

    // make sure the file wasn't overwritten with a folder before (fixes a bug)
    QFileInfo finfo(m_listFilePath);
    if(finfo.isDir())
    {
        QDir badDir(m_listFilePath);
        badDir.removeRecursively();
    }

    qDebug() << "Writing account list to" << m_listFilePath;

    qDebug() << "Building JSON data structure.";
    // Build the JSON document to write to the list file.
    QJsonObject root;

    root.insert("formatVersion", AccountListVersion::MojangMSA);

    // Build a list of accounts.
    qDebug() << "Building account array.";
    QJsonArray accounts;
    for (MinecraftAccountPtr account : m_accounts)
    {
        QJsonObject accountObj = account->saveToJson();
        if(m_activeAccount == account) {
            accountObj["active"] = true;
        }
        accounts.append(accountObj);
    }

    // Insert the account list into the root object.
    root.insert("accounts", accounts);

    // Create a JSON document object to convert our JSON to bytes.
    QJsonDocument doc(root);

    // Now that we're done building the JSON object, we can write it to the file.
    qDebug() << "Writing account list to file.";
    QSaveFile file(m_listFilePath);

    // Try to open the file and fail if we can't.
    // TODO: We should probably report this error to the user.
    if (!file.open(QIODevice::WriteOnly))
    {
        qCritical() << QString("Failed to read the account list file (%1).").arg(m_listFilePath).toUtf8();
        return false;
    }

    // Write the JSON to the file.
    file.write(doc.toJson());
    file.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ReadUser|QFile::WriteUser);
    if(file.commit()) {
        qDebug() << "Saved account list to" << m_listFilePath;
        return true;
    }
    else {
        qDebug() << "Failed to save accounts to" << m_listFilePath;
        return false;
    }
}

void AccountList::setListFilePath(QString path, bool autosave)
{
    m_listFilePath = path;
    m_autosave = autosave;
}

bool AccountList::anyAccountIsValid()
{
    for(auto account:m_accounts)
    {
        if(account->accountStatus() != NotVerified)
            return true;
    }
    return false;
}
