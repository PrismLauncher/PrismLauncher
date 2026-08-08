// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Lenny McLennington <lenny@sneed.church>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "GuiUtil.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFileDialog>
#include <QStandardPaths>

#include "FileSystem.h"
#include "logs/AnonymizeLog.h"
#include "net/NetJob.h"
#include "net/PasteUpload.h"
#include "net/Request.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

#include <BuildConfig.h>
#include <DesktopServices.h>
#include <settings/SettingsObject.h>
#include "Application.h"

constexpr int g_MaxMclogsLines = 25000;
constexpr int g_InitialMclogsLines = 10000;
constexpr int g_FinalMclogsLines = 14900;

namespace {
QString truncateLogForMclogs(const QString& logContent)
{
    QStringList lines = logContent.split("\n");
    if (lines.size() > g_MaxMclogsLines) {
        QString truncatedLog = lines.mid(0, g_InitialMclogsLines).join("\n");
        truncatedLog +=
            "\n\n\n\n\n\n\n\n\n\n"
            "------------------------------------------------------------\n"
            "----------------------- Log truncated ----------------------\n"
            "------------------------------------------------------------\n"
            "----- Middle portion omitted to fit mclo.gs size limits ----\n"
            "------------------------------------------------------------\n"
            "\n\n\n\n\n\n\n\n\n\n";
        truncatedLog += lines.mid(lines.size() - g_FinalMclogsLines - 1).join("\n");
        return truncatedLog;
    }
    return logContent;
}
}  // namespace

std::optional<QString> GuiUtil::uploadPaste(const QString& name, const QFileInfo& filePath, QWidget* parentWidget)
{
    return uploadPaste(name, FS::read(filePath.absoluteFilePath()), parentWidget);
};

std::optional<QString> GuiUtil::uploadPaste(const QString& name, const QString& data, QWidget* parentWidget)
{
    ProgressDialog dialog(parentWidget);
    auto pasteType = static_cast<PasteUpload::PasteType>(APPLICATION->settings()->get("PastebinType").toInt());
    auto baseURL = APPLICATION->settings()->get("PastebinCustomAPIBase").toString();
    bool shouldTruncate = false;

    if (baseURL.isEmpty()) {
        baseURL = PasteUpload::g_PasteTypes.at(static_cast<std::size_t>(pasteType)).defaultBase;
    }

    auto url = QUrl(baseURL);
    if (!url.isValid()) {
        return {};
    }

    auto response = CustomMessageBox::selectable(parentWidget, QObject::tr("Confirm Upload"),
                                                 QObject::tr("You are about to upload \"%1\" to %2.\n"
                                                             "You should double-check for personal information.\n\n"
                                                             "Are you sure?")
                                                     .arg(name, url.host()),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();

    if (response != QMessageBox::Yes) {
        return {};
    }

    if (pasteType == PasteUpload::PasteType::Mclogs && data.count("\n") > g_MaxMclogsLines) {
        auto truncateResponse =
            CustomMessageBox::selectable(parentWidget, QObject::tr("Confirm Truncation"),
                                         QObject::tr("The log has %1 lines, exceeding mclo.gs' limit of %2.\n"
                                                     "The launcher can keep the first %3 and last %4 lines, trimming the middle.\n\n"
                                                     "If you choose 'No', mclo.gs will only keep the first %2 lines, cutting off "
                                                     "potentially useful info like crashes at the end.\n\n"
                                                     "Proceed with truncation?")
                                             .arg(data.count("\n"))
                                             .arg(g_MaxMclogsLines)
                                             .arg(g_InitialMclogsLines)
                                             .arg(g_FinalMclogsLines),
                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No)
                ->exec();

        if (truncateResponse == QMessageBox::Cancel) {
            return {};
        }
        shouldTruncate = truncateResponse == QMessageBox::Yes;
    }

    QString textToUpload = data;
    if (shouldTruncate) {
        textToUpload = truncateLogForMclogs(data);
    }

    auto job = NetJob::Ptr(new NetJob("Log Upload", APPLICATION->network()));

    auto [pasteJob, pasteLink] = PasteUpload::make(textToUpload, baseURL, pasteType);
    job->addNetAction(pasteJob);
    QObject::connect(job.get(), &Task::failed, parentWidget, [parentWidget](const QString& reason) {
        CustomMessageBox::selectable(parentWidget, QObject::tr("Failed to upload logs!"), reason, QMessageBox::Critical)->show();
    });

    if (dialog.execWithTask(job.get()) == QDialog::Accepted) {
        if (pasteLink->isEmpty()) {
            CustomMessageBox::selectable(parentWidget, QObject::tr("Failed to upload logs!"), "The upload link is empty",
                                         QMessageBox::Critical)
                ->show();
            return {};
        }
        setClipboardText(*pasteLink);
        CustomMessageBox::selectable(
            parentWidget, QObject::tr("Upload finished"),
            QObject::tr("The <a href=\"%1\">link to the uploaded log</a> has been placed in your clipboard.").arg(*pasteLink),
            QMessageBox::Information)
            ->exec();
        return *pasteLink;
    }
    return {};
}

void GuiUtil::setClipboardText(QString text)
{
    anonymizeLog(text);
    QApplication::clipboard()->setText(text);
}

namespace {
QStringList browseForFileInternal(const QString& context,
                                  const QString& caption,
                                  const QString& filter,
                                  const QString& defaultPath,
                                  QWidget* parentWidget,
                                  bool single)
{
    static QMap<QString, QString> s_savedPaths;

    QFileDialog w(parentWidget, caption);
    QSet<QString> locations;
    auto f = [&locations](QStandardPaths::StandardLocation l) {
        QString location = QStandardPaths::writableLocation(l);
        QFileInfo finfo(location);
        if (!finfo.exists()) {
            return;
        }
        locations.insert(location);
    };
    f(QStandardPaths::DesktopLocation);
    f(QStandardPaths::DocumentsLocation);
    f(QStandardPaths::DownloadLocation);
    f(QStandardPaths::HomeLocation);
    QList<QUrl> urls;
    for (const auto& location : locations) {
        urls.append(QUrl::fromLocalFile(location));
    }
    urls.append(QUrl::fromLocalFile(defaultPath));

    w.setFileMode(single ? QFileDialog::ExistingFile : QFileDialog::ExistingFiles);
    w.setAcceptMode(QFileDialog::AcceptOpen);
    w.setNameFilter(filter);

    QString pathToOpen;
    if (s_savedPaths.contains(context)) {
        pathToOpen = s_savedPaths[context];
    } else {
        pathToOpen = defaultPath;
    }
    if (!pathToOpen.isEmpty()) {
        QFileInfo finfo(pathToOpen);
        if (finfo.exists() && finfo.isDir()) {
            w.setDirectory(finfo.absoluteFilePath());
        }
    }

    w.setSidebarUrls(urls);

    if (w.exec() != 0) {
        s_savedPaths[context] = w.directory().absolutePath();
        return w.selectedFiles();
    }
    s_savedPaths[context] = w.directory().absolutePath();
    return {};
}
}  // namespace

QString GuiUtil::browseForFile(const QString& context,
                               const QString& caption,
                               const QString& filter,
                               const QString& defaultPath,
                               QWidget* parentWidget)
{
    auto resultList = browseForFileInternal(context, caption, filter, defaultPath, parentWidget, true);
    if (!resultList.isEmpty()) {
        return resultList[0];
    }
    return {};
}

QStringList GuiUtil::browseForFiles(const QString& context,
                                    const QString& caption,
                                    const QString& filter,
                                    const QString& defaultPath,
                                    QWidget* parentWidget)
{
    return browseForFileInternal(context, caption, filter, defaultPath, parentWidget, false);
}
