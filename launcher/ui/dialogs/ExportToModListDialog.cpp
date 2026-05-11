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
#include "Markdown.h"
#include "StringUtils.h"
#include "modplatform/helpers/ExportToModList.h"
#include "ui_ExportToModListDialog.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include <utility>

namespace {
const QHash<ExportToModList::Formats, QString>& exampleLines()
{
    static const QHash<ExportToModList::Formats, QString> s_lines = {
        { ExportToModList::HTML, "<li><a href=\"{url}\">{name}</a> [{version}] by {authors}</li>" },
        { ExportToModList::MARKDOWN, "[{name}]({url}) [{version}] by {authors}" },
        { ExportToModList::PLAINTXT, "{name} ({url}) [{version}] by {authors}" },
        { ExportToModList::JSON, R"({"name":"{name}","url":"{url}","version":"{version}","authors":"{authors}"},)" },
        { ExportToModList::CSV, "{name},{url},{version},\"{authors}\"" },
    };
    return s_lines;
}
}  // namespace

ExportToModListDialog::ExportToModListDialog(QString name, QList<Mod*> mods, QWidget* parent)
    : QDialog(parent), m_mods(std::move(mods)), m_templateChanged(false), m_name(std::move(name)), m_ui(new Ui::ExportToModListDialog)
{
    m_ui->setupUi(this);
    enableCustom(false);

    connect(m_ui->formatComboBox, &QComboBox::currentIndexChanged, this, &ExportToModListDialog::formatChanged);
    connect(m_ui->authorsCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(m_ui->versionCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(m_ui->urlCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(m_ui->filenameCheckBox, &QCheckBox::stateChanged, this, &ExportToModListDialog::trigger);
    connect(m_ui->authorsButton, &QPushButton::clicked, this, [this](bool) { addExtra(ExportToModList::Authors); });
    connect(m_ui->versionButton, &QPushButton::clicked, this, [this](bool) { addExtra(ExportToModList::Version); });
    connect(m_ui->urlButton, &QPushButton::clicked, this, [this](bool) { addExtra(ExportToModList::Url); });
    connect(m_ui->filenameButton, &QPushButton::clicked, this, [this](bool) { addExtra(ExportToModList::FileName); });
    connect(m_ui->templateText, &QTextEdit::textChanged, this, [this] {
        if (m_ui->templateText->toPlainText() != exampleLines().value(m_format)) {
            m_ui->formatComboBox->setCurrentIndex(5);
        }
        triggerImp();
    });
    connect(m_ui->copyButton, &QPushButton::clicked, this, [this](bool) {
        this->m_ui->finalText->selectAll();
        this->m_ui->finalText->copy();
    });

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    triggerImp();
}

ExportToModListDialog::~ExportToModListDialog()
{
    delete m_ui;
}

void ExportToModListDialog::formatChanged(int index)
{
    switch (index) {
        case 0: {
            enableCustom(false);
            m_ui->resultText->show();
            m_format = ExportToModList::HTML;
            break;
        }
        case 1: {
            enableCustom(false);
            m_ui->resultText->show();
            m_format = ExportToModList::MARKDOWN;
            break;
        }
        case 2: {
            enableCustom(false);
            m_ui->resultText->hide();
            m_format = ExportToModList::PLAINTXT;
            break;
        }
        case 3: {
            enableCustom(false);
            m_ui->resultText->hide();
            m_format = ExportToModList::JSON;
            break;
        }
        case 4: {
            enableCustom(false);
            m_ui->resultText->hide();
            m_format = ExportToModList::CSV;
            break;
        }
        case 5: {
            m_templateChanged = true;
            enableCustom(true);
            m_ui->resultText->hide();
            m_format = ExportToModList::CUSTOM;
            break;
        }
        default:
            break;
    }
    triggerImp();
}

void ExportToModListDialog::triggerImp()
{
    if (m_format == ExportToModList::CUSTOM) {
        m_ui->finalText->setPlainText(ExportToModList::exportToModList(m_mods, m_ui->templateText->toPlainText()));
        return;
    }
    auto opt = ExportToModList::OptionalDatas{};
    if (m_ui->authorsCheckBox->isChecked()) {
        opt |= ExportToModList::Authors;
    }
    if (m_ui->versionCheckBox->isChecked()) {
        opt |= ExportToModList::Version;
    }
    if (m_ui->urlCheckBox->isChecked()) {
        opt |= ExportToModList::Url;
    }
    if (m_ui->filenameCheckBox->isChecked()) {
        opt |= ExportToModList::FileName;
    }
    auto txt = ExportToModList::exportToModList(m_mods, m_format, opt);
    m_ui->finalText->setPlainText(txt);
    switch (m_format) {
        case ExportToModList::CUSTOM:
            return;
        case ExportToModList::HTML:
            m_ui->resultText->setHtml(StringUtils::htmlListPatch(txt));
            break;
        case ExportToModList::MARKDOWN:
            m_ui->resultText->setHtml(StringUtils::htmlListPatch(markdownToHTML(txt)));
            break;
        default:
            break;
    }
    auto exampleLine = exampleLines().value(m_format);
    if (!m_templateChanged && m_ui->templateText->toPlainText() != exampleLine) {
        m_ui->templateText->setPlainText(exampleLine);
    }
}

void ExportToModListDialog::done(int result)
{
    if (result == Accepted) {
        const QString filename = FS::RemoveInvalidFilenameChars(m_name);
        const QString output =
            QFileDialog::getSaveFileName(this, tr("Export %1").arg(m_name), FS::PathCombine(QDir::homePath(), filename + extension()),
                                         tr("File") + " (*.txt *.html *.md *.json *.csv)", nullptr);

        if (output.isEmpty()) {
            return;
        }

        try {
            FS::write(output, m_ui->finalText->toPlainText().toUtf8());
        } catch (const FS::FileSystemException& e) {
            qCritical() << "Failed to save mod list file :" << e.cause();
        }
    }

    QDialog::done(result);
}

QString ExportToModListDialog::extension()
{
    switch (m_format) {
        case ExportToModList::HTML:
            return ".html";
        case ExportToModList::MARKDOWN:
            return ".md";
        case ExportToModList::PLAINTXT:
            /*fallthrough*/
        case ExportToModList::CUSTOM:
            return ".txt";
        case ExportToModList::JSON:
            return ".json";
        case ExportToModList::CSV:
            return ".csv";
    }
    return ".txt";
}

void ExportToModListDialog::addExtra(ExportToModList::OptionalData option)
{
    if (m_format != ExportToModList::CUSTOM) {
        return;
    }
    switch (option) {
        case ExportToModList::Authors:
            m_ui->templateText->insertPlainText("{authors}");
            break;
        case ExportToModList::Url:
            m_ui->templateText->insertPlainText("{url}");
            break;
        case ExportToModList::Version:
            m_ui->templateText->insertPlainText("{version}");
            break;
        case ExportToModList::FileName:
            m_ui->templateText->insertPlainText("{filename}");
            break;
    }
}
void ExportToModListDialog::enableCustom(bool enabled)
{
    m_ui->authorsCheckBox->setHidden(enabled);
    m_ui->authorsButton->setHidden(!enabled);

    m_ui->versionCheckBox->setHidden(enabled);
    m_ui->versionButton->setHidden(!enabled);

    m_ui->urlCheckBox->setHidden(enabled);
    m_ui->urlButton->setHidden(!enabled);

    m_ui->filenameCheckBox->setHidden(enabled);
    m_ui->filenameButton->setHidden(!enabled);
}
