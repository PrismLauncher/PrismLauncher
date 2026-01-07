// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "OtherLogsPage.h"
#include "ui_OtherLogsPage.h"

#include <QMessageBox>

#include "ui/GuiUtil.h"
#include "ui/themes/ThemeManager.h"

#include <FileSystem.h>
#include <GZip.h>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QShortcut>
#include <QUrl>

OtherLogsPage::OtherLogsPage(QString id, QString displayName, QString helpPage, BaseInstance* instance, QWidget* parent)
    : QWidget(parent)
    , m_id(id)
    , m_displayName(displayName)
    , m_helpPage(helpPage)
    , ui(new Ui::OtherLogsPage)
    , m_instance(instance)
    , m_basePath(instance ? instance->gameRoot() : APPLICATION->dataRoot())
    , m_logSearchPaths(instance ? instance->getLogFileSearchPaths() : QStringList{ "logs" })
{
    ui->setupUi(this);

    m_proxy = new LogFormatProxyModel(this);
    if (m_instance) {
        m_model = new LogModel(this);
        ui->trackLogCheckbox->hide();
    } else {
        m_model = APPLICATION->logModel.get();
    }

    // set up fonts in the log proxy
    {
        QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
        bool conversionOk = false;
        int fontSize = APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
        if (!conversionOk) {
            fontSize = 11;
        }
        m_proxy->setFont(QFont(fontFamily, fontSize));
    }

    ui->text->setModel(m_proxy);

    if (m_instance) {
        m_model->setMaxLines(getConsoleMaxLines(m_instance->settings()));
        m_model->setStopOnOverflow(shouldStopOnConsoleOverflow(m_instance->settings()));
        m_model->setOverflowMessage(tr("Cannot display this log since the log length surpassed %1 lines.").arg(m_model->getMaxLines()));
    } else {
        modelStateToUI();
    }
    m_proxy->setSourceModel(m_model);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &OtherLogsPage::populateSelectLogBox);

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, &QShortcut::activated, this, &OtherLogsPage::findActivated);

    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, &QShortcut::activated, this, &OtherLogsPage::findNextActivated);

    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, &QShortcut::activated, this, &OtherLogsPage::findPreviousActivated);

    connect(ui->searchBar, &QLineEdit::returnPressed, this, &OtherLogsPage::on_findButton_clicked);
}

OtherLogsPage::~OtherLogsPage()
{
    delete ui;
}

void OtherLogsPage::modelStateToUI()
{
    if (m_model->wrapLines()) {
        ui->text->setWordWrap(true);
        ui->wrapCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setWordWrap(false);
        ui->wrapCheckbox->setCheckState(Qt::Unchecked);
    }
    if (m_model->colorLines()) {
        ui->text->setColorLines(true);
        ui->colorCheckbox->setCheckState(Qt::Checked);
    } else {
        ui->text->setColorLines(false);
        ui->colorCheckbox->setCheckState(Qt::Unchecked);
    }
    if (m_model->suspended()) {
        ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
    } else {
        ui->trackLogCheckbox->setCheckState(Qt::Checked);
    }
}

void OtherLogsPage::UIToModelState()
{
    if (!m_model) {
        return;
    }
    m_model->setLineWrap(ui->wrapCheckbox->checkState() == Qt::Checked);
    m_model->setColorLines(ui->colorCheckbox->checkState() == Qt::Checked);
    m_model->suspend(ui->trackLogCheckbox->checkState() != Qt::Checked);
}

void OtherLogsPage::retranslate()
{
    ui->retranslateUi(this);
}

void OtherLogsPage::openedImpl()
{
    const QStringList failedPaths = m_watcher.addPaths(m_logSearchPaths);

    for (const QString& path : m_logSearchPaths) {
        if (failedPaths.contains(path))
            qDebug() << "Failed to start watching" << path;
        else
            qDebug() << "Started watching" << path;
    }

    populateSelectLogBox();
}

void OtherLogsPage::closedImpl()
{
    const QStringList failedPaths = m_watcher.removePaths(m_logSearchPaths);

    for (const QString& path : m_logSearchPaths) {
        if (failedPaths.contains(path))
            qDebug() << "Failed to stop watching" << path;
        else
            qDebug() << "Stopped watching" << path;
    }
}

void OtherLogsPage::populateSelectLogBox()
{
    const QString prevCurrentFile = m_currentFile;

    ui->selectLogBox->blockSignals(true);
    ui->selectLogBox->clear();
    if (!m_instance)
        ui->selectLogBox->addItem("Current logs");
    ui->selectLogBox->addItems(getPaths());
    ui->selectLogBox->blockSignals(false);

    if (!prevCurrentFile.isEmpty()) {
        const int index = ui->selectLogBox->findText(prevCurrentFile);
        if (index != -1) {
            ui->selectLogBox->blockSignals(true);
            ui->selectLogBox->setCurrentIndex(index);
            ui->selectLogBox->blockSignals(false);
            setControlsEnabled(true);
            // don't refresh file
            return;
        } else {
            setControlsEnabled(false);
        }
    } else if (!m_instance) {
        ui->selectLogBox->setCurrentIndex(0);
        setControlsEnabled(true);
    }

    on_selectLogBox_currentIndexChanged(ui->selectLogBox->currentIndex());
}

void OtherLogsPage::on_selectLogBox_currentIndexChanged(const int index)
{
    QString file;
    if (index > 0 || (index == 0 && m_instance)) {
        file = ui->selectLogBox->itemText(index);
    }

    if ((index != 0 || m_instance) && (file.isEmpty() || !QFile::exists(FS::PathCombine(m_basePath, file)))) {
        m_currentFile = QString();
        ui->text->clear();
        setControlsEnabled(false);
    } else {
        m_currentFile = file;
        reload();
        setControlsEnabled(true);
    }
}

void OtherLogsPage::on_btnReload_clicked()
{
    if (!m_instance && m_currentFile.isEmpty()) {
        if (!m_model)
            return;
        m_model->clear();
        if (m_container)
            m_container->refreshContainer();
    } else {
        reload();
    }
}

void OtherLogsPage::reload()
{
    if (m_currentFile.isEmpty()) {
        if (m_instance) {
            setControlsEnabled(false);
        } else {
            m_model = APPLICATION->logModel.get();
            m_proxy->setSourceModel(m_model);
            ui->text->setModel(m_proxy);
            ui->text->scrollToBottom();
            UIToModelState();
            setControlsEnabled(true);
        }
        return;
    }

    QFile file(FS::PathCombine(m_basePath, m_currentFile));
    if (!file.open(QFile::ReadOnly)) {
        setControlsEnabled(false);
        ui->btnReload->setEnabled(true);  // allow reload
        m_currentFile = QString();
        QMessageBox::critical(this, tr("Error"), tr("Unable to open %1 for reading: %2").arg(m_currentFile, file.errorString()));
    } else {
        auto setPlainText = [this](const QString& text) {
            QTextDocument* doc = ui->text->document();
            doc->setDefaultFont(m_proxy->getFont());
            ui->text->setPlainText(text);
        };
        auto showTooBig = [setPlainText, &file]() {
            setPlainText(tr("The file (%1) is too big. You may want to open it in a viewer optimized "
                            "for large files.")
                             .arg(file.fileName()));
        };
        if (file.size() > (1024ll * 1024ll * 12ll)) {
            showTooBig();
            return;
        }
        MessageLevel last = MessageLevel::Unknown;

        auto handleLine = [this, &last](QString line) {
            if (line.isEmpty())
                return false;
            if (line.back() == '\n')
                line = line.remove(line.size() - 1, 1);
            MessageLevel level = MessageLevel::Unknown;

            QString lineTemp = line;  // don't edit out the time and level for clarity
            if (!m_instance) {
                level = MessageLevel::takeFromLauncherLine(lineTemp);
            } else {
                level = LogParser::guessLevel(line, last);
            }

            last = level;
            m_model->append(level, line);
            return m_model->isOverFlow();
        };

        // Try to determine a level for each line
        ui->text->clear();
        ui->text->setModel(nullptr);
        if (!m_instance) {
            m_model = new LogModel(this);
            m_model->setMaxLines(getConsoleMaxLines(APPLICATION->settings()));
            m_model->setStopOnOverflow(shouldStopOnConsoleOverflow(APPLICATION->settings()));
            m_model->setOverflowMessage(tr("Cannot display this log since the log length surpassed %1 lines.").arg(m_model->getMaxLines()));
        }
        m_model->clear();
        if (file.fileName().endsWith(".gz")) {
            QString line;
            auto error = GZip::readGzFileByBlocks(&file, [&line, handleLine](const QByteArray& d) {
                auto block = d;
                int newlineIndex = block.indexOf('\n');
                while (newlineIndex != -1) {
                    line += QString::fromUtf8(block).left(newlineIndex);
                    block.remove(0, newlineIndex + 1);
                    if (handleLine(line)) {
                        line.clear();
                        return false;
                    }
                    line.clear();
                    newlineIndex = block.indexOf('\n');
                }
                line += QString::fromUtf8(block);
                return true;
            });
            if (!error.isEmpty()) {
                setPlainText(tr("The file (%1) encountered an error when reading: %2.").arg(file.fileName(), error));
                return;
            } else if (!line.isEmpty()) {
                handleLine(line);
            }
        } else {
            while (!file.atEnd() && !handleLine(QString::fromUtf8(file.readLine()))) {
            }
        }

        if (m_instance) {
            ui->text->setModel(m_proxy);
            ui->text->scrollToBottom();
        } else {
            m_proxy->setSourceModel(m_model);
            ui->text->setModel(m_proxy);
            ui->text->scrollToBottom();
            UIToModelState();
            setControlsEnabled(true);
        }
    }
}

void OtherLogsPage::on_btnPaste_clicked()
{
    QString name = m_currentFile.isEmpty() ? displayName() : m_currentFile;
    GuiUtil::uploadPaste(name, ui->text->toPlainText(), this);
}

void OtherLogsPage::on_btnCopy_clicked()
{
    GuiUtil::setClipboardText(ui->text->toPlainText());
}

void OtherLogsPage::on_btnBottom_clicked()
{
    ui->text->scrollToBottom();
}

void OtherLogsPage::on_trackLogCheckbox_clicked(bool checked)
{
    if (!m_model)
        return;
    m_model->suspend(!checked);
}

void OtherLogsPage::on_btnDelete_clicked()
{
    if (m_currentFile.isEmpty()) {
        setControlsEnabled(false);
        return;
    }
    if (QMessageBox::question(this, tr("Confirm Deletion"),
                              tr("You are about to delete \"%1\".\n"
                                 "This may be permanent and it will be gone from the logs folder.\n\n"
                                 "Are you sure?")
                                  .arg(m_currentFile),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        return;
    }
    QFile file(FS::PathCombine(m_basePath, m_currentFile));

    if (FS::trash(file.fileName())) {
        return;
    }

    if (!file.remove()) {
        QMessageBox::critical(this, tr("Error"), tr("Unable to delete %1: %2").arg(m_currentFile, file.errorString()));
    }
}

void OtherLogsPage::on_btnClean_clicked()
{
    auto toDelete = getPaths();
    if (toDelete.isEmpty()) {
        return;
    }
    QMessageBox* messageBox = new QMessageBox(this);
    messageBox->setWindowTitle(tr("Confirm Cleanup"));
    if (toDelete.size() > 5) {
        messageBox->setText(tr("Are you sure you want to delete all log files?"));
        messageBox->setDetailedText(toDelete.join('\n'));
    } else {
        messageBox->setText(tr("Are you sure you want to delete all these files?\n%1").arg(toDelete.join('\n')));
    }
    messageBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    messageBox->setDefaultButton(QMessageBox::Ok);
    messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageBox->setIcon(QMessageBox::Question);
    messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);

    if (messageBox->exec() != QMessageBox::Ok) {
        return;
    }
    QStringList failed;
    for (auto item : toDelete) {
        QString absolutePath = FS::PathCombine(m_basePath, item);
        QFile file(absolutePath);
        qDebug() << "Deleting log" << absolutePath;
        if (FS::trash(file.fileName())) {
            continue;
        }
        if (!file.remove()) {
            failed.push_back(item);
        }
    }
    if (!failed.empty()) {
        QMessageBox* messageBoxFailure = new QMessageBox(this);
        messageBoxFailure->setWindowTitle(tr("Error"));
        if (failed.size() > 5) {
            messageBoxFailure->setText(tr("Couldn't delete some files!"));
            messageBoxFailure->setDetailedText(failed.join('\n'));
        } else {
            messageBoxFailure->setText(tr("Couldn't delete some files:\n%1").arg(failed.join('\n')));
        }
        messageBoxFailure->setStandardButtons(QMessageBox::Ok);
        messageBoxFailure->setDefaultButton(QMessageBox::Ok);
        messageBoxFailure->setTextInteractionFlags(Qt::TextSelectableByMouse);
        messageBoxFailure->setIcon(QMessageBox::Critical);
        messageBoxFailure->setTextInteractionFlags(Qt::TextBrowserInteraction);
        messageBoxFailure->exec();
    }
}

void OtherLogsPage::on_wrapCheckbox_clicked(bool checked)
{
    ui->text->setWordWrap(checked);
    if (!m_model)
        return;
    m_model->setLineWrap(checked);
    ui->text->scrollToBottom();
}

void OtherLogsPage::on_colorCheckbox_clicked(bool checked)
{
    ui->text->setColorLines(checked);
    if (!m_model)
        return;
    m_model->setColorLines(checked);
    ui->text->scrollToBottom();
}

void OtherLogsPage::setControlsEnabled(const bool enabled)
{
    if (m_instance) {
        ui->btnDelete->setEnabled(enabled);
        ui->btnClean->setEnabled(enabled);
    } else if (!m_currentFile.isEmpty()) {
        ui->btnReload->setText("&Reload");
        ui->btnReload->setToolTip("Reload the contents of the log from the disk");
        ui->btnDelete->setEnabled(enabled);
        ui->btnClean->setEnabled(enabled);
        ui->trackLogCheckbox->setEnabled(false);
    } else {
        ui->btnReload->setText("Clear");
        ui->btnReload->setToolTip("Clear the log");
        ui->btnDelete->setEnabled(false);
        ui->btnClean->setEnabled(false);
        ui->trackLogCheckbox->setEnabled(enabled);
    }

    ui->btnReload->setEnabled(enabled);
    ui->btnCopy->setEnabled(enabled);
    ui->btnPaste->setEnabled(enabled);
    ui->text->setEnabled(enabled);
}

QStringList OtherLogsPage::getPaths()
{
    QDir baseDir(m_basePath);

    QStringList result;

    for (QString searchPath : m_logSearchPaths) {
        QDir searchDir(searchPath);

        QStringList filters{ "*.log", "*.log.gz" };

        if (searchPath != m_basePath)
            filters.append("*.txt");

        QStringList entries = searchDir.entryList(filters, QDir::Files | QDir::Readable, QDir::SortFlag::Time);

        for (const QString& name : entries)
            result.append(baseDir.relativeFilePath(searchDir.filePath(name)));
    }

    return result;
}

void OtherLogsPage::on_findButton_clicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    ui->text->findNext(ui->searchBar->text(), reverse);
}

void OtherLogsPage::findNextActivated()
{
    ui->text->findNext(ui->searchBar->text(), false);
}

void OtherLogsPage::findPreviousActivated()
{
    ui->text->findNext(ui->searchBar->text(), true);
}

void OtherLogsPage::findActivated()
{
    // focus the search bar if it doesn't have focus
    if (!ui->searchBar->hasFocus()) {
        ui->searchBar->setFocus();
        ui->searchBar->selectAll();
    }
}
