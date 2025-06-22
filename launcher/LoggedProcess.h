// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022,2023 Sefa Eyeoglu <contact@scrumplex.net>
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

#pragma once

#include <QProcess>
#include <QTextDecoder>
#include "MessageLevel.h"

/*
 * This is a basic process.
 * It has line-based logging support and hides some of the nasty bits.
 */
class LoggedProcess : public QProcess {
    Q_OBJECT
   public:
    enum State { NotRunning, Starting, FailedToStart, Running, Finished, Crashed, Aborted };

   public:
    explicit LoggedProcess(const QTextCodec* output_codec = QTextCodec::codecForLocale(), QObject* parent = 0);
    virtual ~LoggedProcess();

    State state() const;
    int exitCode() const;

    void setDetachable(bool detachable);

   signals:
    void log(QStringList lines, MessageLevel::Enum level);
    void stateChanged(LoggedProcess::State state);

   public slots:
    /**
     * @brief kill the process - equivalent to kill -9
     */
    void kill();

   private slots:
    void on_stdErr();
    void on_stdOut();
    void on_exit(int exit_code, QProcess::ExitStatus status);
    void on_error(QProcess::ProcessError error);
    void on_stateChange(QProcess::ProcessState);

   private:
    void changeState(LoggedProcess::State state);

    QStringList reprocess(const QByteArray& data, QTextDecoder& decoder);

   private:
    QTextDecoder m_err_decoder;
    QTextDecoder m_out_decoder;
    QString m_leftover_line;
    bool m_killed = false;
    State m_state = NotRunning;
    int m_exit_code = 0;
    bool m_is_aborting = false;
    bool m_is_detachable = false;
};
