// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

#include "DevelopingModPage.h"
#include "ui_DevelopingModPage.h"

#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QListWidgetItem>
#include <QVariant>

#include "FileSystem.h"
#include "minecraft/mod/DevelopingModWatcher.h"

namespace {
constexpr int kKindRole = Qt::UserRole;
constexpr int kPathRole = Qt::UserRole + 1;
}  // namespace

DevelopingModPage::DevelopingModPage(MinecraftInstance* inst, QWidget* parent)
    : QWidget(parent), ui(new Ui::DevelopingModPage), m_inst(inst)
{
    ui->setupUi(this);

    connect(ui->enableGroup, &QGroupBox::toggled, this, &DevelopingModPage::enableToggled);
    connect(ui->addFolderButton, &QPushButton::clicked, this, &DevelopingModPage::addFolder);
    connect(ui->addJarButton, &QPushButton::clicked, this, &DevelopingModPage::addJar);
    connect(ui->removeButton, &QPushButton::clicked, this, &DevelopingModPage::removeSelected);
    connect(ui->syncNowButton, &QPushButton::clicked, this, &DevelopingModPage::syncNowClicked);

    loadFromSettings();

    if (auto* watcher = m_inst->developingModWatcher()) {
        connect(watcher, &DevelopingModWatcher::statusChanged, this, &DevelopingModPage::updateStatusLabel);
        updateStatusLabel(watcher->statusText());
    }
}

DevelopingModPage::~DevelopingModPage()
{
    delete ui;
}

void DevelopingModPage::openedImpl()
{
    loadFromSettings();
    if (auto* watcher = m_inst->developingModWatcher())
        updateStatusLabel(watcher->statusText());
}

QString DevelopingModPage::displayText(WatchKind kind, const QString& path)
{
    if (kind == WatchKind::Folder)
        return QObject::tr("Folder — %1").arg(path);
    return QObject::tr("Jar — %1").arg(path);
}

void DevelopingModPage::setControlsEnabled(bool enabled)
{
    ui->watchList->setEnabled(enabled);
    ui->addFolderButton->setEnabled(enabled);
    ui->addJarButton->setEnabled(enabled);
    ui->removeButton->setEnabled(enabled);
    ui->ignoreEdit->setEnabled(enabled);
    ui->syncNowButton->setEnabled(enabled);
}

void DevelopingModPage::addWatchItem(WatchKind kind, const QString& path)
{
    const auto normalized = FS::NormalizePath(path);
    if (normalized.isEmpty() || containsPath(normalized))
        return;

    auto* item = new QListWidgetItem(displayText(kind, normalized), ui->watchList);
    item->setData(kKindRole, static_cast<int>(kind));
    item->setData(kPathRole, normalized);
}

bool DevelopingModPage::containsPath(const QString& path) const
{
    const auto normalized = FS::NormalizePath(path);
    for (int i = 0; i < ui->watchList->count(); ++i) {
        if (FS::NormalizePath(ui->watchList->item(i)->data(kPathRole).toString()) == normalized)
            return true;
    }
    return false;
}

QStringList DevelopingModPage::pathsOfKind(WatchKind kind) const
{
    QStringList paths;
    for (int i = 0; i < ui->watchList->count(); ++i) {
        auto* item = ui->watchList->item(i);
        if (static_cast<WatchKind>(item->data(kKindRole).toInt()) != kind)
            continue;
        const auto path = FS::NormalizePath(item->data(kPathRole).toString());
        if (!path.isEmpty())
            paths << path;
    }
    paths.removeDuplicates();
    return paths;
}

void DevelopingModPage::loadFromSettings()
{
    auto* settings = m_inst->settings();

    ui->enableGroup->blockSignals(true);
    ui->ignoreEdit->blockSignals(true);

    ui->enableGroup->setChecked(settings->get("DevelopingModEnabled").toBool());
    ui->watchList->clear();

    auto loadJsonPaths = [this](const QString& raw, WatchKind kind) {
        QJsonParseError error{};
        const auto doc = QJsonDocument::fromJson(raw.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isArray())
            return;
        for (const auto& value : doc.array()) {
            const auto path = value.toString().trimmed();
            if (!path.isEmpty())
                addWatchItem(kind, path);
        }
    };

    loadJsonPaths(settings->get("DevelopingModFolders").toString().trimmed(), WatchKind::Folder);
    loadJsonPaths(settings->get("DevelopingModJars").toString().trimmed(), WatchKind::Jar);

    auto ignore = settings->get("DevelopingModIgnorePatterns").toString();
    if (ignore.trimmed().isEmpty())
        ignore = DevelopingModWatcher::defaultIgnorePatterns().join(QLatin1Char('\n'));
    ui->ignoreEdit->setPlainText(ignore);

    ui->enableGroup->blockSignals(false);
    ui->ignoreEdit->blockSignals(false);

    setControlsEnabled(ui->enableGroup->isChecked());
}

void DevelopingModPage::saveWatchTargets()
{
    QJsonArray folders;
    for (const auto& folder : pathsOfKind(WatchKind::Folder))
        folders.append(folder);
    m_inst->settings()->set("DevelopingModFolders", QString::fromUtf8(QJsonDocument(folders).toJson(QJsonDocument::Compact)));

    QJsonArray jars;
    for (const auto& jar : pathsOfKind(WatchKind::Jar))
        jars.append(jar);
    m_inst->settings()->set("DevelopingModJars", QString::fromUtf8(QJsonDocument(jars).toJson(QJsonDocument::Compact)));
}

void DevelopingModPage::saveIgnorePatterns()
{
    m_inst->settings()->set("DevelopingModIgnorePatterns", ui->ignoreEdit->toPlainText());
}

void DevelopingModPage::enableToggled(bool checked)
{
    m_inst->settings()->set("DevelopingModEnabled", checked);
    setControlsEnabled(checked);
}

void DevelopingModPage::addFolder()
{
    const auto path = QFileDialog::getExistingDirectory(this, tr("Select build output folder"), QDir::homePath());
    if (path.isEmpty())
        return;

    addWatchItem(WatchKind::Folder, path);
    saveWatchTargets();
}

void DevelopingModPage::addJar()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Select JAR to watch"), QDir::homePath(),
                                                   tr("JAR files (*.jar);;All files (*)"));
    if (path.isEmpty())
        return;

    addWatchItem(WatchKind::Jar, path);
    saveWatchTargets();
}

void DevelopingModPage::removeSelected()
{
    const auto items = ui->watchList->selectedItems();
    if (items.isEmpty())
        return;

    for (auto* item : items)
        delete item;
    saveWatchTargets();
}

void DevelopingModPage::syncNowClicked()
{
    apply();
    if (auto* watcher = m_inst->developingModWatcher())
        watcher->syncNow();
}

void DevelopingModPage::updateStatusLabel(const QString& status)
{
    ui->statusLabel->setText(tr("Status: %1").arg(status));
}

bool DevelopingModPage::apply()
{
    m_inst->settings()->set("DevelopingModEnabled", ui->enableGroup->isChecked());
    saveWatchTargets();
    saveIgnorePatterns();
    return true;
}

void DevelopingModPage::retranslate()
{
    ui->retranslateUi(this);
    // Refresh list labels for current language.
    for (int i = 0; i < ui->watchList->count(); ++i) {
        auto* item = ui->watchList->item(i);
        const auto kind = static_cast<WatchKind>(item->data(kKindRole).toInt());
        const auto path = item->data(kPathRole).toString();
        item->setText(displayText(kind, path));
    }
}
