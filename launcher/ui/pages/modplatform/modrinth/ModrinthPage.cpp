// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ModrinthPage.h"
#include "ui_ModrinthPage.h"

#include <QKeyEvent>

#include "Application.h"
#include "InstanceImportTask.h"
#include "Json.h"
#include "ModDownloadTask.h"
#include "ModrinthModel.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

ModrinthPage::ModrinthPage(ModDownloadDialog *dialog, BaseInstance *instance)
    : QWidget(dialog), m_instance(instance), ui(new Ui::ModrinthPage),
      dialog(dialog) {
  ui->setupUi(this);
  connect(ui->searchButton, &QPushButton::clicked, this,
          &ModrinthPage::triggerSearch);
  ui->searchEdit->installEventFilter(this);
  listModel = new Modrinth::ListModel(this);
  ui->packView->setModel(listModel);

  ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(
      Qt::ScrollBarAsNeeded);
  ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

  // index is used to set the sorting with the modrinth api
  ui->sortByBox->addItem(tr("Sort by Relevence"));
  ui->sortByBox->addItem(tr("Sort by Downloads"));
  ui->sortByBox->addItem(tr("Sort by Follows"));
  ui->sortByBox->addItem(tr("Sort by last updated"));
  ui->sortByBox->addItem(tr("Sort by newest"));

  connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(triggerSearch()));
  connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged,
          this, &ModrinthPage::onSelectionChanged);
  connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this,
          &ModrinthPage::onVersionSelectionChanged);
  connect(ui->modSelectionButton, &QPushButton::clicked, this,
          &ModrinthPage::onModSelected);
}

ModrinthPage::~ModrinthPage() { delete ui; }

bool ModrinthPage::eventFilter(QObject *watched, QEvent *event) {
  if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Return) {
      triggerSearch();
      keyEvent->accept();
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

bool ModrinthPage::shouldDisplay() const { return true; }

void ModrinthPage::retranslate() {
    ui->retranslateUi(this);
}

void ModrinthPage::openedImpl() {
  updateSelectionButton();
  triggerSearch();
}

void ModrinthPage::triggerSearch() {
  listModel->searchWithTerm(ui->searchEdit->text(),
                            ui->sortByBox->currentIndex());
}

void ModrinthPage::onSelectionChanged(QModelIndex first, QModelIndex second) {
  ui->versionSelectionBox->clear();

  if (!first.isValid()) {
    return;
  }

  current = listModel->data(first, Qt::UserRole).value<Modrinth::IndexedPack>();
  QString text = "";
  QString name = current.name;

  if (current.websiteUrl.isEmpty())
    text = name;
  else
    text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
  text += "<br>" + tr(" by ") + "<a href=\"" + current.author.url + "\">" +
          current.author.name + "</a><br><br>";
  ui->packDescription->setHtml(text + current.description);

  if (!current.versionsLoaded) {
    qDebug() << "Loading Modrinth mod versions";

    ui->modSelectionButton->setText(tr("Loading versions..."));
    ui->modSelectionButton->setEnabled(false);

    auto netJob =
        new NetJob(QString("Modrinth::ModVersions(%1)").arg(current.name),
                   APPLICATION->network());
    auto response = new QByteArray();
    QString addonId = current.addonId;
    netJob->addNetAction(Net::Download::makeByteArray(
        QString("https://api.modrinth.com/v2/project/%1/version").arg(addonId),
        response));

    QObject::connect(netJob, &NetJob::succeeded, this, [this, response, addonId] {
        if(addonId != current.addonId){
            return;
        }
      QJsonParseError parse_error;
      QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
      if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Modrinth at "
                   << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return;
      }
      QJsonArray arr = doc.array();
      try {
        Modrinth::loadIndexedPackVersions(current, arr, APPLICATION->network(),
                                          m_instance);
      } catch (const JSONValidationError &e) {
        qDebug() << *response;
        qWarning() << "Error while reading Modrinth mod version: " << e.cause();
      }
      auto packProfile = ((MinecraftInstance *)m_instance)->getPackProfile();
      QString mcVersion = packProfile->getComponentVersion("net.minecraft");
      QString loaderString =
          (packProfile->getComponentVersion("net.minecraftforge").isEmpty())
              ? "fabric"
              : "forge";
      for (int i = 0; i < current.versions.size(); i++) {
        auto version = current.versions[i];
        if (!version.mcVersion.contains(mcVersion) ||
            !version.loaders.contains(loaderString)) {
          continue;
        }
        ui->versionSelectionBox->addItem(version.version, QVariant(i));
      }
      if (ui->versionSelectionBox->count() == 0) {
        ui->versionSelectionBox->addItem(tr("No Valid Version found !"),
                                         QVariant(-1));
      }

      ui->modSelectionButton->setText(tr("Cannot select invalid version :("));
      updateSelectionButton();
    });

    QObject::connect(netJob, &NetJob::finished, this, [response, netJob] {
      netJob->deleteLater();
      delete response;
    });

    netJob->start();
  } else {
    for (int i = 0; i < current.versions.size(); i++) {
      ui->versionSelectionBox->addItem(current.versions[i].version,
                                       QVariant(i));
    }
    if (ui->versionSelectionBox->count() == 0) {
      ui->versionSelectionBox->addItem(tr("No Valid Version found !"),
                                       QVariant(-1));
    }

    updateSelectionButton();
  }
}

void ModrinthPage::updateSelectionButton() {
  if (!isOpened || selectedVersion < 0) {
    ui->modSelectionButton->setEnabled(false);
    return;
  }

  ui->modSelectionButton->setEnabled(true);
  auto &version = current.versions[selectedVersion];
  if (!dialog->isModSelected(current.name, version.fileName)) {
    ui->modSelectionButton->setText(tr("Select mod for download"));
  } else {
    ui->modSelectionButton->setText(tr("Deselect mod for download"));
  }
}

void ModrinthPage::onVersionSelectionChanged(QString data) {
  if (data.isNull() || data.isEmpty()) {
    selectedVersion = -1;
    return;
  }
  selectedVersion = ui->versionSelectionBox->currentData().toInt();
  updateSelectionButton();
}

void ModrinthPage::onModSelected() {
  auto &version = current.versions[selectedVersion];
  if (dialog->isModSelected(current.name, version.fileName)) {
    dialog->removeSelectedMod(current.name);
  } else {
    dialog->addSelectedMod(current.name,
                           new ModDownloadTask(version.downloadUrl,
                                               version.fileName, dialog->mods));
  }

  updateSelectionButton();
}
