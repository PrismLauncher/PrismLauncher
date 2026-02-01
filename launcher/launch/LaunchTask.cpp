// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *      Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "launch/LaunchTask.h"
#include <assert.h>
#include <QAnyStringView>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <variant>
#include "MessageLevel.h"
#include "tasks/Task.h"

void LaunchTask::init()
{
    m_instance->setRunning(true);
}

std::unique_ptr<LaunchTask> LaunchTask::create(MinecraftInstance* inst)
{
    auto task = std::unique_ptr<LaunchTask>(new LaunchTask(inst));
    task->init();
    return task;
}

LaunchTask::LaunchTask(MinecraftInstance* instance) : m_instance(instance) {}

void LaunchTask::appendStep(shared_qobject_ptr<LaunchStep> step)
{
    m_steps.append(step);
}

void LaunchTask::prependStep(shared_qobject_ptr<LaunchStep> step)
{
    m_steps.prepend(step);
}

void LaunchTask::executeTask()
{
    m_instance->setCrashed(false);
    if (!m_steps.size()) {
        state = LaunchTask::Finished;
        emitSucceeded();
        return;
    }
    state = LaunchTask::Running;
    onStepFinished();
}

void LaunchTask::onReadyForLaunch()
{
    state = LaunchTask::Waiting;
    emit readyForLaunch();
}

void LaunchTask::onStepFinished()
{
    // initial -> just start the first step
    if (currentStep == -1) {
        currentStep++;
        m_steps[currentStep]->start();
        return;
    }

    auto step = m_steps[currentStep];
    if (step->wasSuccessful()) {
        // end?
        if (currentStep == m_steps.size() - 1) {
            finalizeSteps(true, QString());
        } else {
            currentStep++;
            step = m_steps[currentStep];
            step->start();
        }
    } else {
        finalizeSteps(false, step->failReason());
    }
}

void LaunchTask::finalizeSteps(bool successful, const QString& error)
{
    for (auto step = currentStep; step >= 0; step--) {
        m_steps[step]->finalize();
    }
    if (successful) {
        emitSucceeded();
    } else {
        emitFailed(error);
    }
}

void LaunchTask::onProgressReportingRequested()
{
    state = LaunchTask::Waiting;
    emit requestProgress(m_steps[currentStep].get());
}

void LaunchTask::setCensorFilter(QMap<QString, QString> filter)
{
    m_censorFilter = filter;
}

QString LaunchTask::censorPrivateInfo(QString in)
{
    auto iter = m_censorFilter.begin();
    while (iter != m_censorFilter.end()) {
        in.replace(iter.key(), iter.value());
        iter++;
    }
    return in;
}

void LaunchTask::proceed()
{
    if (state != LaunchTask::Waiting) {
        return;
    }
    m_steps[currentStep]->proceed();
}

bool LaunchTask::canAbort() const
{
    switch (state) {
        case LaunchTask::Aborted:
        case LaunchTask::Failed:
        case LaunchTask::Finished:
            return false;
        case LaunchTask::NotStarted:
            return true;
        case LaunchTask::Running:
        case LaunchTask::Waiting: {
            auto step = m_steps[currentStep];
            return step->canAbort();
        }
    }
    return false;
}

bool LaunchTask::abort()
{
    switch (state) {
        case LaunchTask::Aborted:
        case LaunchTask::Failed:
        case LaunchTask::Finished:
            return true;
        case LaunchTask::NotStarted: {
            state = LaunchTask::Aborted;
            emitFailed("Aborted");
            return true;
        }
        case LaunchTask::Running:
        case LaunchTask::Waiting: {
            auto step = m_steps[currentStep];
            if (!step->canAbort()) {
                return false;
            }
            if (step->abort()) {
                state = LaunchTask::Aborted;
                return true;
            }
        }
        default:
            break;
    }
    return false;
}

shared_qobject_ptr<LogModel> LaunchTask::getLogModel()
{
    if (!m_logModel) {
        m_logModel.reset(new LogModel());
        m_logModel->setMaxLines(getConsoleMaxLines(m_instance->settings()));
        m_logModel->setStopOnOverflow(shouldStopOnConsoleOverflow(m_instance->settings()));
        // FIXME: should this really be here?
        m_logModel->setOverflowMessage(tr("Stopped watching the game log because the log length surpassed %1 lines.\n"
                                          "You may have to fix your mods because the game is still logging to files and"
                                          " likely wasting harddrive space at an alarming rate!")
                                           .arg(m_logModel->getMaxLines()));
    }
    return m_logModel;
}

bool LaunchTask::parseXmlLogs(QString const& line, MessageLevel level)
{
    LogParser* parser;
    switch (static_cast<MessageLevel::Enum>(level)) {
        case MessageLevel::StdErr:
            parser = &m_stderrParser;
            break;
        case MessageLevel::StdOut:
            parser = &m_stdoutParser;
            break;
        default:
            return false;
    }

    parser->appendLine(line);
    auto items = parser->parseAvailable();
    if (auto err = parser->getError(); err.has_value()) {
        auto& model = *getLogModel();
        model.append(MessageLevel::Error, tr("[Log4j Parse Error] Failed to parse log4j log event: %1").arg(err.value().errMessage));
        return false;
    }

    if (items.isEmpty())
        return true;

    auto model = getLogModel();
    for (auto const& item : items) {
        if (std::holds_alternative<LogParser::LogEntry>(item)) {
            auto entry = std::get<LogParser::LogEntry>(item);
            auto msg = QString("[%1] [%2/%3] [%4]: %5")
                           .arg(entry.timestamp.toString("HH:mm:ss"))
                           .arg(entry.thread)
                           .arg(entry.levelText)
                           .arg(entry.logger)
                           .arg(entry.message);
            msg = censorPrivateInfo(msg);
            model->append(entry.level, msg);
        } else if (std::holds_alternative<LogParser::PlainText>(item)) {
            auto msg = std::get<LogParser::PlainText>(item).message;

            MessageLevel newLevel = MessageLevel::takeFromLine(msg);

            if (newLevel == MessageLevel::Unknown)
                newLevel = LogParser::guessLevel(line, model->previousLevel());

            msg = censorPrivateInfo(msg);

            model->append(newLevel, msg);
        }
    }

    return true;
}

void LaunchTask::onLogLines(const QStringList& lines, MessageLevel defaultLevel)
{
    for (auto& line : lines) {
        onLogLine(line, defaultLevel);
    }
}

void LaunchTask::onLogLine(QString line, MessageLevel level)
{
    if (parseXmlLogs(line, level)) {
        return;
    }

    // censor private user info
    line = censorPrivateInfo(line);

    getLogModel()->append(level, line);
}

void LaunchTask::emitSucceeded()
{
    m_instance->setRunning(false);
    Task::emitSucceeded();
}

void LaunchTask::emitFailed(QString reason)
{
    m_instance->setRunning(false);
    m_instance->setCrashed(true);
    Task::emitFailed(reason);
}

QString expandVariables(const QString& input, QProcessEnvironment dict)
{
    QString result = input;

    enum { base, maybeBrace, variable, brace } state = base;
    int startIdx = -1;
    for (int i = 0; i < result.length();) {
        QChar c = result.at(i++);
        switch (state) {
            case base:
                if (c == '$')
                    state = maybeBrace;
                break;
            case maybeBrace:
                if (c == '{') {
                    state = brace;
                    startIdx = i;
                } else if (c.isLetterOrNumber() || c == '_') {
                    state = variable;
                    startIdx = i - 1;
                } else {
                    state = base;
                }
                break;
            case brace:
                if (c == '}') {
                    const auto res = dict.value(result.mid(startIdx, i - 1 - startIdx), "");
                    if (!res.isEmpty()) {
                        result.replace(startIdx - 2, i - startIdx + 2, res);
                        i = startIdx - 2 + res.length();
                    }
                    state = base;
                }
                break;
            case variable:
                if (!c.isLetterOrNumber() && c != '_') {
                    const auto res = dict.value(result.mid(startIdx, i - startIdx - 1), "");
                    if (!res.isEmpty()) {
                        result.replace(startIdx - 1, i - startIdx, res);
                        i = startIdx - 1 + res.length();
                    }
                    state = base;
                }
                break;
        }
    }
    if (state == variable) {
        if (const auto res = dict.value(result.mid(startIdx), ""); !res.isEmpty())
            result.replace(startIdx - 1, result.length() - startIdx + 1, res);
    }
    return result;
}

QString LaunchTask::substituteVariables(QString& cmd, bool isLaunch) const
{
    return expandVariables(cmd, isLaunch ? m_instance->createLaunchEnvironment() : m_instance->createEnvironment());
}
