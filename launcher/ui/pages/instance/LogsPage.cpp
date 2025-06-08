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

#include "LogsPage.h"
#include "ui_LogsPage.h"

#include "launch/LaunchTask.h"
#include "ui/GuiUtil.h"
#include "ui/LogFormatProxyModel.h"
#include "ui/themes/ThemeManager.h"

#include <FileSystem.h>
#include <GZip.h>
#include <logs/LogParser.h>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QIdentityProxyModel>
#include <QMessageBox>
#include <QShortcut>
#include <QUrl>

LogsPage::LogsPage(InstancePtr instance, QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::LogsPage)
    , m_instance(instance)
    , m_basePath(instance ? instance->gameRoot() : APPLICATION->dataRoot())
    , m_logSearchPaths(instance ? instance->getLogFileSearchPaths() : QStringList{ "logs" })
{
    m_ui->setupUi(this);

    m_proxy = new LogFormatProxyModel(this);

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

    m_ui->text->setModel(m_proxy);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &LogsPage::populateSelectLogBox);

    auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
    connect(findShortcut, &QShortcut::activated, this, &LogsPage::findActivated);

    auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
    connect(findNextShortcut, &QShortcut::activated, this, &LogsPage::findNextActivated);

    auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
    connect(findPreviousShortcut, &QShortcut::activated, this, &LogsPage::findPreviousActivated);

    connect(m_ui->searchBar, &QLineEdit::returnPressed, this, &LogsPage::findClicked);

    connect(m_ui->selectLogBox, &QComboBox::currentIndexChanged, this, &LogsPage::loadLog);
    connect(m_ui->cleanUpButton, &QAbstractButton::clicked, this, &LogsPage::cleanUpClicked);

    connect(m_ui->copyButton, &QAbstractButton::clicked, this, &LogsPage::copyClicked);
    connect(m_ui->uploadButton, &QAbstractButton::clicked, this, &LogsPage::uploadClicked);
    connect(m_ui->deleteButton, &QAbstractButton::clicked, this, &LogsPage::deleteClicked);
    connect(m_ui->reloadButton, &QAbstractButton::clicked, this, &LogsPage::loadLog);
    connect(m_ui->clearButton, &QAbstractButton::clicked, this, &LogsPage::clearClicked);

    connect(m_ui->keepUpdatingCheckbox, &QCheckBox::toggled, this, &LogsPage::keepUpdatingToggled);
    connect(m_ui->wrapLinesCheckbox, &QCheckBox::toggled, this, &LogsPage::wrapCheckboxToggled);
    connect(m_ui->colorLinesCheckbox, &QCheckBox::toggled, this, &LogsPage::colorLinesToggled);

    if (instance != nullptr) {
        connect(instance.get(), &BaseInstance::launchTaskChanged, this, [this] {
            if (currentFile().isNull())
                loadLog();
        });
    }

    loadLog();
}

LogsPage::~LogsPage()
{
    delete m_ui;
}

void LogsPage::retranslate()
{
    m_ui->retranslateUi(this);
}

void LogsPage::openedImpl()
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

void LogsPage::closedImpl()
{
    const QStringList failedPaths = m_watcher.removePaths(m_logSearchPaths);

    for (const QString& path : m_logSearchPaths) {
        if (failedPaths.contains(path))
            qDebug() << "Failed to stop watching" << path;
        else
            qDebug() << "Stopped watching" << path;
    }
}

void LogsPage::useModel(shared_qobject_ptr<LogModel> model)
{
    m_model.reset(model);
    m_proxy->setSourceModel(model.get());
}

void LogsPage::useModel(LogModel* model)
{
    m_model.reset(model);
    m_proxy->setSourceModel(model);
}

QString LogsPage::currentFile() const
{
    if (m_ui->selectLogBox->currentIndex() == 0)
        return QString();
    else
        return m_ui->selectLogBox->currentText();
}

void LogsPage::populateSelectLogBox()
{
    const QString prevFile = currentFile();

    m_ui->selectLogBox->blockSignals(true);
    m_ui->selectLogBox->clear();
    m_ui->selectLogBox->addItem(tr("Current Log"));
    m_ui->selectLogBox->addItems(getPaths());
    m_ui->selectLogBox->blockSignals(false);

    if (!prevFile.isEmpty()) {
        const int index = m_ui->selectLogBox->findText(prevFile);
        if (index != -1) {
            m_ui->selectLogBox->blockSignals(true);
            m_ui->selectLogBox->setCurrentIndex(index);
            m_ui->selectLogBox->blockSignals(false);
        } else
            loadLog();
    }
}

void LogsPage::loadLog()
{
    QString file = currentFile();

    if (file.isNull())
        loadCurrentLog();
    else
        loadLogFile(file);

    m_ui->text->scrollToBottom();
}

void LogsPage::loadCurrentLog()
{
    if (m_instance != nullptr) {
        const auto launchTask = m_instance->getLaunchTask();

        if (launchTask) {
            useModel(launchTask->getLogModel());
        } else {
            useModel(new LogModel(this));
            m_model->append(MessageLevel::Info, "The game log will appear here after launching.");
        }
    } else
        m_model = APPLICATION->logModel;

    m_ui->clearButton->setVisible(true);
    m_ui->keepUpdatingCheckbox->setVisible(true);
    m_ui->keepUpdatingCheckbox->setChecked(!m_model->suspended());

    m_ui->deleteButton->setVisible(false);
    m_ui->reloadButton->setVisible(false);
}

void LogsPage::loadLogFile(const QString& path)
{
    m_ui->text->setModel(nullptr);
    auto cleanup = qScopeGuard([this] { m_ui->text->setModel(m_proxy); });

    useModel(new LogModel(this));
    m_model->setMaxLines(getConsoleMaxLines(APPLICATION->settings()));
    m_model->setStopOnOverflow(shouldStopOnConsoleOverflow(APPLICATION->settings()));
    m_model->setOverflowMessage(tr("Cannot display this log file since its length surpassed %1 lines.").arg(m_model->getMaxLines()));

    m_ui->deleteButton->setVisible(true);
    m_ui->reloadButton->setVisible(true);

    m_ui->clearButton->setVisible(false);
    m_ui->keepUpdatingCheckbox->setVisible(false);

    QFile file(FS::PathCombine(m_basePath, path));
    if (!file.open(QFile::ReadOnly)) {
        QString errorMessage = tr("Unable to open %1 for reading: %2").arg(path, file.errorString());
        m_model->append(MessageLevel::Fatal, std::move(errorMessage));
        return;
    }

    if (file.size() > (1024ll * 1024ll * 12ll)) {
        QString errorMessage = tr("The file (%1) is too big. Please open it in a viewer optimized for large files.").arg(file.fileName());
        m_model->append(MessageLevel::Fatal, std::move(errorMessage));
        return;
    }

    MessageLevel::Enum last = MessageLevel::Unknown;

    auto handleLine = [this, &last](QString line) {
        if (line.isEmpty())
            return false;
        if (line.back() == '\n')
            line = line.remove(line.size() - 1, 1);
        MessageLevel::Enum level = MessageLevel::Unknown;

        QString lineTemp = line;  // don't edit out the time and level for clarity
        if (!m_instance) {
            level = MessageLevel::fromLauncherLine(lineTemp);
        } else {
            // if the launcher part set a log level, use it
            auto innerLevel = MessageLevel::fromLine(lineTemp);
            if (innerLevel != MessageLevel::Unknown) {
                level = innerLevel;
            }

            // If the level is still undetermined, guess level
            if (level == MessageLevel::StdErr || level == MessageLevel::StdOut || level == MessageLevel::Unknown) {
                level = LogParser::guessLevel(line, last);
            }
        }

        last = level;
        m_model->append(level, line);
        return m_model->isOverFlow();
    };

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
            QString errorMessage = tr("The file (%1) encountered an error when reading: %2.").arg(file.fileName(), error);
            m_model->append(MessageLevel::Fatal, std::move(errorMessage));
            return;
        } else if (!line.isEmpty()) {
            handleLine(line);
        }
    } else {
        while (!file.atEnd() && !handleLine(QString::fromUtf8(file.readLine()))) {
        }
    }
}

void LogsPage::uploadClicked()
{
    GuiUtil::uploadPaste(m_ui->selectLogBox->currentText(), m_ui->text->toPlainText(), this);
}

void LogsPage::copyClicked()
{
    GuiUtil::setClipboardText(m_ui->text->toPlainText());
}

void LogsPage::keepUpdatingToggled(bool checked)
{
    if (!m_model)
        return;
    m_model->suspend(!checked);
}

void LogsPage::deleteClicked()
{
    QString current = m_ui->selectLogBox->currentText();

    if (QMessageBox::question(this, tr("Confirm Deletion"),
                              tr("You are about to delete \"%1\".\n"
                                 "This may be permanent and it will be gone from the logs folder.\n\n"
                                 "Are you sure?")
                                  .arg(current),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        return;
    }
    QFile file(FS::PathCombine(m_basePath, current));

    if (FS::trash(file.fileName())) {
        return;
    }

    if (!file.remove())
        QMessageBox::critical(this, tr("Error"), tr("Unable to delete %1: %2").arg(current, file.errorString()));
}

void LogsPage::clearClicked()
{
    if (!m_model)
        return;

    m_model.clear();
}

void LogsPage::cleanUpClicked()
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

void LogsPage::wrapCheckboxToggled(bool checked)
{
    m_ui->text->setWordWrap(checked);
    if (!m_model)
        return;
    m_model->setLineWrap(checked);
}

void LogsPage::colorLinesToggled(bool checked)
{
    m_ui->text->setColorLines(checked);
    if (!m_model)
        return;
    m_model->setColorLines(checked);
    m_ui->text->scrollToBottom();
}

QStringList LogsPage::getPaths() const
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

void LogsPage::findClicked()
{
    auto modifiers = QApplication::keyboardModifiers();
    bool reverse = modifiers & Qt::ShiftModifier;
    m_ui->text->findNext(m_ui->searchBar->text(), reverse);
}

void LogsPage::findNextActivated()
{
    m_ui->text->findNext(m_ui->searchBar->text(), false);
}

void LogsPage::findPreviousActivated()
{
    m_ui->text->findNext(m_ui->searchBar->text(), true);
}

void LogsPage::findActivated()
{
    // focus the search bar if it doesn't have focus
    if (!m_ui->searchBar->hasFocus()) {
        m_ui->searchBar->setFocus();
        m_ui->searchBar->selectAll();
    }
}
