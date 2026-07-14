// SPDX-FileCopyrightText: 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "PrismExternalUpdater.h"

#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSettings>
#include <QTimer>
#include <algorithm>
#include <memory>

#include "StringUtils.h"

#include "BuildConfig.h"

#include "ui/dialogs/UpdateAvailableDialog.h"

class PrismExternalUpdater::Private {
   public:
    QDir appDir;
    QDir dataDir;
    QTimer updateTimer;
    bool allowBeta{};
    bool autoCheck{};
    double updateInterval{};
    QDateTime lastCheck;
    std::unique_ptr<QSettings> settings;

    QWidget* parent{};
};

PrismExternalUpdater::PrismExternalUpdater(QWidget* parent, const QString& appDir, const QString& dataDir)
    : priv(new PrismExternalUpdater::Private())
{
    priv->appDir = QDir(appDir);
    priv->dataDir = QDir(dataDir);
    auto settingsFile = priv->dataDir.absoluteFilePath("prismlauncher_update.cfg");
    priv->settings = std::make_unique<QSettings>(settingsFile, QSettings::Format::IniFormat);
    priv->allowBeta = priv->settings->value("allow_beta", false).toBool();
    priv->autoCheck = priv->settings->value("auto_check", true).toBool();
    bool intervalOk = false;
    // default once per day
    priv->updateInterval = priv->settings->value("update_interval", 86400).toInt(&intervalOk);
    if (!intervalOk) {
        priv->updateInterval = 86400;
    }
    if (const auto lastCheck = priv->settings->value("last_check"); !lastCheck.isNull() && lastCheck.isValid()) {
        priv->lastCheck = QDateTime::fromString(lastCheck.toString(), Qt::ISODate);
    }
    priv->parent = parent;
    connectTimer();
    resetAutoCheckTimer();
    if (priv->updateInterval == 0) {  // "On Launch"
        checkForUpdates(false);
    }
}

PrismExternalUpdater::~PrismExternalUpdater()
{
    if (priv->updateTimer.isActive()) {
        priv->updateTimer.stop();
    }
    disconnectTimer();
    priv->settings->sync();
    delete priv;
}

void PrismExternalUpdater::checkForUpdates()
{
    checkForUpdates(true);
}

void PrismExternalUpdater::checkForUpdates(bool triggeredByUser) const
{
    QProgressDialog progress(tr("Checking for updates..."), "", 0, 0, priv->parent);
    progress.setMinimumDuration(0); // Appear immediately without waiting
    progress.setCancelButton(nullptr);
    progress.adjustSize();
    if (triggeredByUser) {
        progress.show();
    }
    QCoreApplication::processEvents();

    QProcess proc;
    auto exeName = QStringLiteral("%1_updater").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
#ifdef Q_OS_WIN32
    exeName.append(".exe");

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("__COMPAT_LAYER", "RUNASINVOKER");
    proc.setProcessEnvironment(env);
#else
    exeName = QString("bin/%1").arg(exeName);
#endif

    QStringList args = { "--check-only", "--dir", priv->dataDir.absolutePath(), "--debug" };
    if (priv->allowBeta) {
        args.append("--pre-release");
    }

    proc.start(priv->appDir.absoluteFilePath(exeName), args);
    if (auto resultStart = proc.waitForStarted(5000); !resultStart) {
        auto err = proc.error();
        qDebug() << "Failed to start updater after 5 seconds."
                 << "reason:" << err << proc.errorString();
        auto msgBox =
            QMessageBox(QMessageBox::Information, tr("Update Check Failed"),
                        tr("Failed to start after 5 seconds\nReason: %1.").arg(proc.errorString()), QMessageBox::Ok, priv->parent);
        msgBox.setMinimumWidth(460);
        msgBox.adjustSize();
        msgBox.exec();
        priv->lastCheck = QDateTime::currentDateTime();
        priv->settings->setValue("last_check", priv->lastCheck.toString(Qt::ISODate));
        priv->settings->sync();
        resetAutoCheckTimer();
        return;
    }
    QCoreApplication::processEvents();

    if (auto resultFinished = proc.waitForFinished(60000); !resultFinished) {
        proc.kill();
        auto err = proc.error();
        auto output = proc.readAll();
        qDebug() << "Updater failed to close after 60 seconds."
                 << "reason:" << err << proc.errorString();
        auto msgBox =
            QMessageBox(QMessageBox::Information, tr("Update Check Failed"),
                        tr("Updater failed to close 60 seconds\nReason: %1.").arg(proc.errorString()), QMessageBox::Ok, priv->parent);
        msgBox.setDetailedText(output);
        msgBox.setMinimumWidth(460);
        msgBox.adjustSize();
        msgBox.exec();
        priv->lastCheck = QDateTime::currentDateTime();
        priv->settings->setValue("last_check", priv->lastCheck.toString(Qt::ISODate));
        priv->settings->sync();
        resetAutoCheckTimer();
        return;
    }

    auto exitCode = proc.exitCode();

    auto stdOutput = proc.readAllStandardOutput();
    auto stdError = proc.readAllStandardError();

    progress.cancel();
    QCoreApplication::processEvents();

    switch (exitCode) {
        case 0:
            // no update available
            if (triggeredByUser) {
                qDebug() << "No update available";
                auto msgBox = QMessageBox(QMessageBox::Information, tr("No Update Available"), tr("You are running the latest version."),
                                          QMessageBox::Ok, priv->parent);
                msgBox.setMinimumWidth(460);
                msgBox.adjustSize();
                msgBox.exec();
            }
            break;
        case 1:
            // there was an error
            {
                qDebug() << "Updater subprocess error" << qPrintable(stdError);
                auto msgBox = QMessageBox(QMessageBox::Warning, tr("Update Check Error"),
                                          tr("There was an error running the update check."), QMessageBox::Ok, priv->parent);
                msgBox.setDetailedText(QString(stdError));
                msgBox.setMinimumWidth(460);
                msgBox.adjustSize();
                msgBox.exec();
            }
            break;
        case 100:
            // update available
            {
                auto [firstLine, remainder1] = StringUtils::splitFirst(stdOutput, '\n');
                auto [secondLine, remainder2] = StringUtils::splitFirst(remainder1, '\n');
                auto [thirdLine, releaseNotes] = StringUtils::splitFirst(remainder2, '\n');
                auto versionName = StringUtils::splitFirst(firstLine, ": ").second.trimmed();
                auto versionTag = StringUtils::splitFirst(secondLine, ": ").second.trimmed();
                auto releaseTimestamp = QDateTime::fromString(StringUtils::splitFirst(thirdLine, ": ").second.trimmed(), Qt::ISODate);
                qDebug() << "Update available:" << versionName << versionTag << releaseTimestamp;
                qDebug() << "Update release notes:" << releaseNotes;

                offerUpdate(versionName, versionTag, releaseNotes, triggeredByUser);
            }
            break;
        default:
            // unknown error code
            {
                qDebug() << "Updater exited with unknown code" << exitCode;
                auto msgBox = QMessageBox(QMessageBox::Information, tr("Unknown Update Error"),
                                          tr("The updater exited with an unknown condition.\nExit Code: %1").arg(QString::number(exitCode)),
                                          QMessageBox::Ok, priv->parent);
                auto detailTxt = tr("StdOut: %1\nStdErr: %2").arg(QString(stdOutput)).arg(QString(stdError));
                msgBox.setDetailedText(detailTxt);
                msgBox.setMinimumWidth(460);
                msgBox.adjustSize();
                msgBox.exec();
            }
    }
    priv->lastCheck = QDateTime::currentDateTime();
    priv->settings->setValue("last_check", priv->lastCheck.toString(Qt::ISODate));
    priv->settings->sync();
    resetAutoCheckTimer();
}

bool PrismExternalUpdater::getAutomaticallyChecksForUpdates()
{
    return priv->autoCheck;
}

double PrismExternalUpdater::getUpdateCheckInterval()
{
    return priv->updateInterval;
}

bool PrismExternalUpdater::getBetaAllowed()
{
    return priv->allowBeta;
}

void PrismExternalUpdater::setAutomaticallyChecksForUpdates(bool check)
{
    priv->autoCheck = check;
    priv->settings->setValue("auto_check", check);
    priv->settings->sync();
    resetAutoCheckTimer();
}

void PrismExternalUpdater::setUpdateCheckInterval(double seconds)
{
    priv->updateInterval = seconds;
    priv->settings->setValue("update_interval", seconds);
    priv->settings->sync();
    resetAutoCheckTimer();
}

void PrismExternalUpdater::setBetaAllowed(bool allowed)
{
    priv->allowBeta = allowed;
    priv->settings->setValue("auto_beta", allowed);
    priv->settings->sync();
}

void PrismExternalUpdater::resetAutoCheckTimer() const
{
    if (priv->autoCheck && priv->updateInterval > 0) {
        auto now = QDateTime::currentDateTime();

        qint64 timeoutMs = 0;

        if (priv->lastCheck.isValid()) {
            const qint64 diff = priv->lastCheck.secsTo(now);
            const qint64 secsLeft = std::max<qint64>(priv->updateInterval - diff, 0);
            timeoutMs = secsLeft * 1000;
        }

        timeoutMs = std::min(timeoutMs, static_cast<qint64>(INT_MAX));

        qDebug() << "Auto update timer starting," << timeoutMs / 1000 << "seconds left";
        priv->updateTimer.start(static_cast<int>(timeoutMs));
    } else {
        if (priv->updateTimer.isActive()) {
            priv->updateTimer.stop();
        }
    }
}

void PrismExternalUpdater::connectTimer()
{
    connect(&priv->updateTimer, &QTimer::timeout, this, &PrismExternalUpdater::autoCheckTimerFired);
}

void PrismExternalUpdater::disconnectTimer()
{
    disconnect(&priv->updateTimer, &QTimer::timeout, this, &PrismExternalUpdater::autoCheckTimerFired);
}

void PrismExternalUpdater::autoCheckTimerFired() const
{
    qDebug() << "Auto update Timer fired";
    checkForUpdates(false);
}

void PrismExternalUpdater::offerUpdate(const QString& versionName,
                                       const QString& versionTag,
                                       const QString& releaseNotes,
                                       const bool triggeredByUser) const
{
    priv->settings->beginGroup("skip");
    auto shouldSkip = !triggeredByUser && priv->settings->value(versionTag, false).toBool();
    priv->settings->endGroup();

    if (shouldSkip) {
        if (triggeredByUser) {
            auto msgBox = QMessageBox(QMessageBox::Information, tr("No Update Available"), tr("There are no new updates available."),
                                      QMessageBox::Ok, priv->parent);
            msgBox.setMinimumWidth(460);
            msgBox.adjustSize();
            msgBox.exec();
        }
        return;
    }

    UpdateAvailableDialog dlg(BuildConfig.printableVersionString(), versionName, releaseNotes);

    auto result = dlg.exec();
    qDebug() << "offer dlg result" << result;

    priv->settings->beginGroup("skip");
    if (result == UpdateAvailableDialog::Skip) {
        priv->settings->setValue(versionTag, true);
    } else {
        if (result == UpdateAvailableDialog::Install) {
            performUpdate(versionTag);
        }
        priv->settings->remove(versionTag);
    }
    priv->settings->endGroup();
    priv->settings->sync();
}

void PrismExternalUpdater::performUpdate(const QString& versionTag) const
{
    QProcess proc;
    auto exeName = QStringLiteral("%1_updater").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
#ifdef Q_OS_WIN32
    exeName.append(".exe");

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("__COMPAT_LAYER", "RUNASINVOKER");
    proc.setProcessEnvironment(env);
#else
    exeName = QString("bin/%1").arg(exeName);
#endif

    QStringList args = { "--dir", priv->dataDir.absolutePath(), "--install-version", versionTag };
    if (priv->allowBeta) {
        args.append("--pre-release");
    }

    proc.setProgram(priv->appDir.absoluteFilePath(exeName));
    proc.setArguments(args);
    auto result = proc.startDetached();
    if (!result) {
        qDebug() << "Failed to start updater:" << proc.error() << proc.errorString();
    }
    QCoreApplication::exit();
}
