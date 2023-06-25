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

#include "ExportToModListDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/helpers/ExportToModList.h"
#include "ui_ExportToModListDialog.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>

ExportToModListDialog::ExportToModListDialog(InstancePtr instance, QWidget* parent)
    : QDialog(parent), m_template_selected(false), name(instance->name()), ui(new Ui::ExportToModListDialog)
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

    connect(ui->formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportToModListDialog::formatChanged);
    connect(ui->authorsCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(ui->versionCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(ui->urlCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(ui->templateText, &QTextEdit::textChanged, this, &ExportToModListDialog::triggerImp);
    connect(ui->copyButton, &QPushButton::clicked, this, [this](bool) {
        this->ui->finalText->selectAll();
        this->ui->finalText->copy();
    });
}

ExportToModListDialog::~ExportToModListDialog()
{
    delete ui;
}

void ExportToModListDialog::formatChanged(int index)
{
    switch (index) {
        case 0: {
            ui->templateGroup->setDisabled(true);
            ui->optionsGroup->setDisabled(false);
            ui->resultText->show();
            format = ExportToModList::HTML;
            break;
        }
        case 1: {
            ui->templateGroup->setDisabled(true);
            ui->optionsGroup->setDisabled(false);
            ui->resultText->show();
            format = ExportToModList::MARKDOWN;
            break;
        }
        case 2: {
            ui->templateGroup->setDisabled(true);
            ui->optionsGroup->setDisabled(false);
            ui->resultText->hide();
            format = ExportToModList::PLAINTXT;
            break;
        }
        case 3: {
            ui->templateGroup->setDisabled(false);
            ui->optionsGroup->setDisabled(true);
            ui->resultText->hide();
            format = ExportToModList::CUSTOM;
            break;
        }
    }
    triggerImp();
}

void ExportToModListDialog::triggerImp()
{
    if (format == ExportToModList::CUSTOM) {
        m_template_selected = true;
        ui->finalText->setPlainText(ExportToModList::ExportToModList(m_allMods, ui->templateText->toPlainText()));
        return;
    }
    auto opt = 0;
    if (ui->authorsCheckBox->isChecked())
        opt |= ExportToModList::Authors;
    if (ui->versionCheckBox->isChecked())
        opt |= ExportToModList::Version;
    if (ui->urlCheckBox->isChecked())
        opt |= ExportToModList::Url;
    auto txt = ExportToModList::ExportToModList(m_allMods, format, static_cast<ExportToModList::OptionalData>(opt));
    ui->finalText->setPlainText(txt);
    QString exampleLine;
    switch (format) {
        case ExportToModList::HTML: {
            exampleLine = "<ul><a href=\"{url}\">{name}</a>[{version}] by {authors}</ul>";
            ui->resultText->setHtml(txt);
            break;
        }
        case ExportToModList::MARKDOWN: {
            exampleLine = "[{name}]({url})[{version}] by {authors}";
            ui->resultText->setMarkdown(txt);
            break;
        }
        case ExportToModList::PLAINTXT: {
            exampleLine = "name: {name}; url: {url}; version: {version}; authors: {authors};";
            break;
        }
        case ExportToModList::CUSTOM:
            return;
    }
    if (!m_template_selected) {
        if (ui->templateText->toPlainText() != exampleLine)
            ui->templateText->setPlainText(exampleLine);
    }
}

void ExportToModListDialog::done(int result)
{
    if (result == Accepted) {
        const QString filename = FS::RemoveInvalidFilenameChars(name);
        const QString output =
            QFileDialog::getSaveFileName(this, tr("Export %1").arg(name), FS::PathCombine(QDir::homePath(), filename + extension()),
                                         "File (*.txt *.html *.md)", nullptr);

        if (output.isEmpty())
            return;
        FS::write(output, ui->finalText->toPlainText().toUtf8());
    }

    QDialog::done(result);
}

QString ExportToModListDialog::extension()
{
    switch (format) {
        case ExportToModList::HTML:
            return ".html";
        case ExportToModList::MARKDOWN:
            return ".md";
        case ExportToModList::PLAINTXT:
            return ".txt";
        case ExportToModList::CUSTOM:
            return ".txt";
    }
    return ".txt";
}
