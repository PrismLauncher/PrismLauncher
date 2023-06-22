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

#include "ExportModsToStringDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/helpers/ExportModsToStringTask.h"
#include "ui_ExportModsToStringDialog.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>

ExportModsToStringDialog::ExportModsToStringDialog(InstancePtr instance, QWidget* parent)
    : QDialog(parent), m_template_selected(false), ui(new Ui::ExportModsToStringDialog)
{
    ui->setupUi(this);
    ui->templateGroup->setDisabled(true);

    MinecraftInstance* mcInstance = dynamic_cast<MinecraftInstance*>(instance.get());
    if (mcInstance) {
        mcInstance->loaderModList()->update();
        connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, [this, mcInstance]() {
            m_allMods = mcInstance->loaderModList()->allMods();
            triggerImp();
        });
    }

    connect(ui->formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportModsToStringDialog::formatChanged);
    connect(ui->authorsCheckBox, &QCheckBox::stateChanged, this, &ExportModsToStringDialog::trigger);
    connect(ui->versionCheckBox, &QCheckBox::stateChanged, this, &ExportModsToStringDialog::trigger);
    connect(ui->urlCheckBox, &QCheckBox::stateChanged, this, &ExportModsToStringDialog::trigger);
    connect(ui->templateText, &QTextEdit::textChanged, this, &ExportModsToStringDialog::triggerImp);
    connect(ui->copyButton, &QPushButton::clicked, this, [this](bool) {
        this->ui->finalText->selectAll();
        this->ui->finalText->copy();
    });
}

ExportModsToStringDialog::~ExportModsToStringDialog()
{
    delete ui;
}

void ExportModsToStringDialog::formatChanged(int index)
{
    switch (index) {
        case 0: {
            ui->templateGroup->setDisabled(true);
            ui->optionsGroup->setDisabled(false);
            break;
        }
        case 1: {
            ui->templateGroup->setDisabled(true);
            ui->optionsGroup->setDisabled(false);
            break;
        }
        case 2: {
            ui->templateGroup->setDisabled(false);
            ui->optionsGroup->setDisabled(true);
            break;
        }
    }
    triggerImp();
}

void ExportModsToStringDialog::triggerImp()
{
    ExportToString::Formats format;
    switch (ui->formatComboBox->currentIndex()) {
        case 2: {
            m_template_selected = true;
            ui->finalText->setPlainText(ExportToString::ExportModsToStringTask(m_allMods, ui->templateText->toPlainText()));
            return;
        }
        case 0: {
            format = ExportToString::HTML;
            break;
        }
        case 1: {
            format = ExportToString::MARKDOWN;
            break;
        }
    }
    auto opt = 0;
    if (ui->authorsCheckBox->isChecked())
        opt |= ExportToString::Authors;
    if (ui->versionCheckBox->isChecked())
        opt |= ExportToString::Version;
    if (ui->urlCheckBox->isChecked())
        opt |= ExportToString::Url;
    ui->finalText->setPlainText(ExportToString::ExportModsToStringTask(m_allMods, format, static_cast<ExportToString::OptionalData>(opt)));
    if (!m_template_selected) {
        auto exampleLine = format == ExportToString::HTML ? "<ul><a href=\"{url}\">{name}</a>[{version}] by {authors}</ul>"
                                                          : "[{name}]({url})[{version}] by {authors}";
        if (ui->templateText->toPlainText() != exampleLine)
            ui->templateText->setPlainText(exampleLine);
    }
}
