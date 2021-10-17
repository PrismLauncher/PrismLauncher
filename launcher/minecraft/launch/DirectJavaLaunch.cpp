/* Copyright 2013-2021 MultiMC Contributors
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

#include "DirectJavaLaunch.h"
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <FileSystem.h>
#include <Commandline.h>
#include <QStandardPaths>

DirectJavaLaunch::DirectJavaLaunch(LaunchTask *parent) : LaunchStep(parent)
{
    connect(&m_process, &LoggedProcess::log, this, &DirectJavaLaunch::logLines);
    connect(&m_process, &LoggedProcess::stateChanged, this, &DirectJavaLaunch::on_state);
}

void DirectJavaLaunch::executeTask()
{
    auto instance = m_parent->instance();
    std::shared_ptr<MinecraftInstance> minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);
    QStringList args = minecraftInstance->javaArguments();

    args.append("-Djava.library.path=" + minecraftInstance->getNativePath());

    auto classPathEntries = minecraftInstance->getClassPath();
    args.append("-cp");
    QString classpath;
#ifdef Q_OS_WIN32
    classpath = classPathEntries.join(';');
#else
    classpath = classPathEntries.join(':');
#endif
    args.append(classpath);
    args.append(minecraftInstance->getMainClass());

    QString allArgs = args.join(", ");
    emit logLine("Java Arguments:\n[" + m_parent->censorPrivateInfo(allArgs) + "]\n\n", MessageLevel::Launcher);

    auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());

    m_process.setProcessEnvironment(instance->createEnvironment());

    // make detachable - this will keep the process running even if the object is destroyed
    m_process.setDetachable(true);

    auto mcArgs = minecraftInstance->processMinecraftArgs(m_session, m_serverToJoin);
    args.append(mcArgs);

    QString wrapperCommandStr = instance->getWrapperCommand().trimmed();
    if(!wrapperCommandStr.isEmpty())
    {
        auto wrapperArgs = Commandline::splitArgs(wrapperCommandStr);
        auto wrapperCommand = wrapperArgs.takeFirst();
        auto realWrapperCommand = QStandardPaths::findExecutable(wrapperCommand);
        if (realWrapperCommand.isEmpty())
        {
            const char *reason = QT_TR_NOOP("The wrapper command \"%1\" couldn't be found.");
            emit logLine(QString(reason).arg(wrapperCommand), MessageLevel::Fatal);
            emitFailed(tr(reason).arg(wrapperCommand));
            return;
        }
        emit logLine("Wrapper command is:\n" + wrapperCommandStr + "\n\n", MessageLevel::Launcher);
        args.prepend(javaPath);
        m_process.start(wrapperCommand, wrapperArgs + args);
    }
    else
    {
        m_process.start(javaPath, args);
    }
}

void DirectJavaLaunch::on_state(LoggedProcess::State state)
{
    switch(state)
    {
        case LoggedProcess::FailedToStart:
        {
            //: Error message displayed if instance can't start
            const char *reason = QT_TR_NOOP("Could not launch minecraft!");
            emit logLine(reason, MessageLevel::Fatal);
            emitFailed(tr(reason));
            return;
        }
        case LoggedProcess::Aborted:
        case LoggedProcess::Crashed:
        {
            m_parent->setPid(-1);
            emitFailed(tr("Game crashed."));
            return;
        }
        case LoggedProcess::Finished:
        {
            m_parent->setPid(-1);
            // if the exit code wasn't 0, report this as a crash
            auto exitCode = m_process.exitCode();
            if(exitCode != 0)
            {
                emitFailed(tr("Game crashed."));
                return;
            }
            //FIXME: make this work again
            // m_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));
            // run post-exit
            emitSucceeded();
            break;
        }
        case LoggedProcess::Running:
            emit logLine(QString("Minecraft process ID: %1\n\n").arg(m_process.processId()), MessageLevel::Launcher);
            m_parent->setPid(m_process.processId());
            m_parent->instance()->setLastLaunch();
            break;
        default:
            break;
    }
}

void DirectJavaLaunch::setWorkingDirectory(const QString &wd)
{
    m_process.setWorkingDirectory(wd);
}

void DirectJavaLaunch::proceed()
{
    // nil
}

bool DirectJavaLaunch::abort()
{
    auto state = m_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
    {
        m_process.kill();
    }
    return true;
}

