// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "SkinList.h"

#include <QFileInfo>
#include <QMimeData>

#include "FileSystem.h"
#include "Json.h"
#include "minecraft/skins/SkinModel.h"

SkinList::SkinList(QObject* parent, QString path, MinecraftAccountPtr acct) : QAbstractListModel(parent), m_acct(acct)
{
    FS::ensureFolderPathExists(m_dir.absolutePath());
    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
    m_watcher.reset(new QFileSystemWatcher(this));
    is_watching = false;
    connect(m_watcher.get(), &QFileSystemWatcher::directoryChanged, this, &SkinList::directoryChanged);
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &SkinList::fileChanged);
    directoryChanged(path);
}

void SkinList::startWatching()
{
    if (is_watching) {
        return;
    }
    update();
    is_watching = m_watcher->addPath(m_dir.absolutePath());
    if (is_watching) {
        qDebug() << "Started watching " << m_dir.absolutePath();
    } else {
        qDebug() << "Failed to start watching " << m_dir.absolutePath();
    }
}

void SkinList::stopWatching()
{
    save();
    if (!is_watching) {
        return;
    }
    is_watching = !m_watcher->removePath(m_dir.absolutePath());
    if (!is_watching) {
        qDebug() << "Stopped watching " << m_dir.absolutePath();
    } else {
        qDebug() << "Failed to stop watching " << m_dir.absolutePath();
    }
}

bool SkinList::update()
{
    QVector<SkinModel> newSkins;
    m_dir.refresh();

    auto manifestInfo = QFileInfo(m_dir.absoluteFilePath("index.json"));
    if (manifestInfo.exists()) {
        try {
            auto doc = Json::requireDocument(manifestInfo.absoluteFilePath(), "SkinList JSON file");
            const auto root = doc.object();
            auto skins = Json::ensureArray(root, "skins");
            for (auto jSkin : skins) {
                SkinModel s(m_dir, Json::ensureObject(jSkin));
                if (s.isValid()) {
                    newSkins << s;
                }
            }
        } catch (const Exception& e) {
            qCritical() << "Couldn't load skins json:" << e.cause();
        }
    } else {
        newSkins = loadMinecraftSkins();
    }

    bool needsSave = false;
    const auto& skin = m_acct->accountData()->minecraftProfile.skin;
    if (!skin.url.isEmpty() && !skin.data.isEmpty()) {
        QPixmap skinTexture;
        SkinModel* nskin = nullptr;
        for (auto i = 0; i < newSkins.size(); i++) {
            if (newSkins[i].getURL() == skin.url) {
                nskin = &newSkins[i];
                break;
            }
        }
        if (!nskin) {
            auto name = m_acct->profileName() + ".png";
            if (QFileInfo(m_dir.absoluteFilePath(name)).exists()) {
                name = QUrl(skin.url).fileName() + ".png";
            }
            auto path = m_dir.absoluteFilePath(name);
            if (skinTexture.loadFromData(skin.data, "PNG") && skinTexture.save(path)) {
                SkinModel s(path);
                s.setModel(SkinModel::CLASSIC);  // maybe better model detection
                s.setCapeId(m_acct->accountData()->minecraftProfile.currentCape);
                s.setURL(skin.url);
                newSkins << s;
                needsSave = true;
            }
        } else {
            nskin->setCapeId(m_acct->accountData()->minecraftProfile.currentCape);
        }
    }

    auto folderContents = m_dir.entryInfoList();
    // if there are any untracked files...
    for (QFileInfo entry : folderContents) {
        if (!entry.isFile() && entry.suffix() != "png")
            continue;

        SkinModel w(entry.absoluteFilePath());
        if (w.isValid()) {
            auto add = true;
            for (auto s : newSkins) {
                if (s.name() == w.name()) {
                    add = false;
                    break;
                }
            }
            if (add) {
                newSkins.append(w);
                needsSave = true;
            }
        }
    }
    std::sort(newSkins.begin(), newSkins.end(),
              [](const SkinModel& a, const SkinModel& b) { return a.getPath().localeAwareCompare(b.getPath()) < 0; });
    beginResetModel();
    m_skin_list.swap(newSkins);
    endResetModel();
    if (needsSave)
        save();
    return true;
}

void SkinList::directoryChanged(const QString& path)
{
    QDir new_dir(path);
    if (!new_dir.exists())
        if (!FS::ensureFolderPathExists(new_dir.absolutePath()))
            return;
    if (m_dir.absolutePath() != new_dir.absolutePath()) {
        m_dir.setPath(path);
        m_dir.refresh();
        if (is_watching)
            stopWatching();
        startWatching();
    }
    update();
}

void SkinList::fileChanged(const QString& path)
{
    qDebug() << "Checking " << path;
    QFileInfo checkfile(path);
    if (!checkfile.exists())
        return;

    for (int i = 0; i < m_skin_list.count(); i++) {
        if (m_skin_list[i].getPath() == checkfile.absoluteFilePath()) {
            m_skin_list[i].refresh();
            dataChanged(index(i), index(i));
            break;
        }
    }
}

QStringList SkinList::mimeTypes() const
{
    return { "text/uri-list" };
}

Qt::DropActions SkinList::supportedDropActions() const
{
    return Qt::CopyAction;
}

bool SkinList::dropMimeData(const QMimeData* data,
                            Qt::DropAction action,
                            [[maybe_unused]] int row,
                            [[maybe_unused]] int column,
                            [[maybe_unused]] const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
        return true;
    // check if the action is supported
    if (!data || !(action & supportedDropActions()))
        return false;

    // files dropped from outside?
    if (data->hasUrls()) {
        auto urls = data->urls();
        QStringList iconFiles;
        for (auto url : urls) {
            // only local files may be dropped...
            if (!url.isLocalFile())
                continue;
            iconFiles += url.toLocalFile();
        }
        installSkins(iconFiles);
        return true;
    }
    return false;
}

Qt::ItemFlags SkinList::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsDropEnabled | QAbstractListModel::flags(index);
    if (index.isValid()) {
        f |= (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }
    return f;
}

QVariant SkinList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();

    if (row < 0 || row >= m_skin_list.size())
        return QVariant();
    auto skin = m_skin_list[row];
    switch (role) {
        case Qt::DecorationRole:
            return skin.getTexture();
        case Qt::DisplayRole:
            return skin.name();
        case Qt::UserRole:
            return skin.name();
        case Qt::EditRole:
            return skin.name();
        default:
            return QVariant();
    }
}

int SkinList::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_skin_list.size();
}

void SkinList::installSkins(const QStringList& iconFiles)
{
    for (QString file : iconFiles)
        installSkin(file, {});
}

void SkinList::installSkin(const QString& file, const QString& name)
{
    QFileInfo fileinfo(file);
    if (!fileinfo.isReadable() || !fileinfo.isFile())
        return;

    if (fileinfo.suffix() != "png" && !SkinModel(fileinfo.absoluteFilePath()).isValid())
        return;

    QString target = FS::PathCombine(m_dir.absolutePath(), name.isEmpty() ? fileinfo.fileName() : name);
    QFile::copy(file, target);
}

int SkinList::getSkinIndex(const QString& key) const
{
    for (int i = 0; i < m_skin_list.count(); i++) {
        if (m_skin_list[i].name() == key) {
            return i;
        }
    }
    return -1;
}

const SkinModel* SkinList::skin(const QString& key) const
{
    int idx = getSkinIndex(key);
    if (idx == -1)
        return nullptr;
    return &m_skin_list[idx];
}

SkinModel* SkinList::skin(const QString& key)
{
    int idx = getSkinIndex(key);
    if (idx == -1)
        return nullptr;
    return &m_skin_list[idx];
}

bool SkinList::deleteSkin(const QString& key, const bool trash)
{
    int idx = getSkinIndex(key);
    if (idx != -1) {
        auto s = m_skin_list[idx];
        if (trash) {
            if (FS::trash(s.getPath(), nullptr)) {
                m_skin_list.remove(idx);
                return true;
            }
        } else if (QFile::remove(s.getPath())) {
            m_skin_list.remove(idx);
            return true;
        }
    }
    return false;
}

void SkinList::save()
{
    QJsonObject doc;
    QJsonArray arr;
    for (auto s : m_skin_list) {
        arr << s.toJSON();
    }
    doc["skins"] = arr;
    Json::write(doc, m_dir.absoluteFilePath("index.json"));
}

int SkinList::getSelectedAccountSkin()
{
    const auto& skin = m_acct->accountData()->minecraftProfile.skin;
    for (int i = 0; i < m_skin_list.count(); i++) {
        if (m_skin_list[i].getURL() == skin.url) {
            return i;
        }
    }
    return -1;
}

bool SkinList::setData(const QModelIndex& idx, const QVariant& value, int role)
{
    if (!idx.isValid() || role != Qt::EditRole) {
        return false;
    }

    int row = idx.row();
    if (row < 0 || row >= m_skin_list.size())
        return false;
    auto skin = m_skin_list[row];
    auto newName = value.toString();
    if (skin.name() != newName) {
        skin.rename(newName);
        save();
    }
    return true;
}

QVector<SkinModel> SkinList::loadMinecraftSkins()
{
    QString partialPath;
#if defined(Q_OS_OSX)
    partialPath = FS::PathCombine(QDir::homePath(), "Library/Application Support");
#elif defined(Q_OS_WIN32)
    partialPath = QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA", "");
#else
    partialPath = QDir::homePath();
#endif
    QVector<SkinModel> newSkins;
    auto path = FS::PathCombine(partialPath, ".minecraft", "launcher_custom_skins.json");
    auto manifestInfo = QFileInfo(path);
    if (!manifestInfo.exists())
        return {};
    try {
        auto doc = Json::requireDocument(manifestInfo.absoluteFilePath(), "SkinList JSON file");
        const auto root = doc.object();
        auto skins = Json::ensureObject(root, "customSkins");
        for (auto key : skins.keys()) {
            SkinModel s(m_dir, Json::ensureObject(skins, key));
            if (s.isValid()) {
                newSkins << s;
            }
        }
    } catch (const Exception& e) {
        qCritical() << "Couldn't load minecraft skins json:" << e.cause();
    }
    return newSkins;
}
