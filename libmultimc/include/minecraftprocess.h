/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MINECRAFTPROCESS_H
#define MINECRAFTPROCESS_H

#include <QProcess>

#include "instance.h"

#include "libmmc_config.h"

/**
 * @file data/minecraftprocess.h
 * @brief The MinecraftProcess class
 */
class LIBMULTIMC_EXPORT MinecraftProcess : public QProcess
{
    Q_OBJECT
public:
    /**
     * @brief MinecraftProcess constructor
     * @param inst the Instance pointer to launch
     * @param user the minecraft username
     * @param session the minecraft session id
     * @param console the instance console window
     */
    MinecraftProcess(InstancePtr inst, QString user, QString session);

    /**
     * @brief launch minecraft
     */
    void launch();

    /**
     * @brief extract the instance icon
     * @param inst the instance
     * @param destination the destination path
     */
    static inline void extractIcon(InstancePtr inst, QString destination);

    /**
     * @brief extract the MultiMC launcher.jar
     * @param destination the destination path
     */
    static inline void extractLauncher(QString destination);

    /**
     * @brief prepare the launch by extracting icon and launcher
     * @param inst the instance
     */
    static void prepare(InstancePtr inst);

    /**
     * @brief split a string into argv items like a shell would do
     * @param args the argument string
     * @return a QStringList containing all arguments
     */
    static QStringList splitArgs(QString args);

signals:
    /**
     * @brief emitted when mc has finished and the PostLaunchCommand was run
     */
    void ended();

protected:
    InstancePtr m_instance;
    QString m_user;
    QString m_session;
    QProcess m_prepostlaunchprocess;
    QStringList m_arguments;

    void genArgs();
	void log(QString text, ConsoleWindow::WriteMode mode = ConsoleWindow::MULTIMC);

protected slots:
    void finish(int, QProcess::ExitStatus status);
    void on_stdErr();
    void on_stdOut();

};

#endif // MINECRAFTPROCESS_H
