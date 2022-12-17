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

#include <QStandardPaths>

#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <FileSystem.h>
#include <Commandline.h>

#include "Application.h"

#ifdef Q_OS_LINUX
#include "gamemode_client.h"
#endif

DirectJavaLaunch::DirectJavaLaunch(LaunchTask *parent) : LaunchStep(parent)
{
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process, &LoggedProcess::log, this, &DirectJavaLaunch::logLines);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process, &LoggedProcess::stateChanged, this, &DirectJavaLaunch::on_state);
}

void DirectJavaLaunch::executeTask()
{
    auto instance = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->instance();
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
    emit logLine("Java Arguments:\n[" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->censorPrivateInfo(allArgs) + "]\n\n", MessageLevel::Launcher);

    auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.setProcessEnvironment(instance->createLaunchEnvironment());

    // make detachable - this will keep the process running even if the object is destroyed
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.setDetachable(true);

    auto mcArgs = minecraftInstance->processMinecraftArgs(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_session, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin);
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
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.start(wrapperCommand, wrapperArgs + args);
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.start(javaPath, args);
    }

#ifdef Q_OS_LINUX
    if (instance->settings()->get("EnableFeralGamemode").toBool() && APPLICATION->capabilities() & Application::SupportsGameMode)
    {
        auto pid = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.processId();
        if (pid)
        {
            gamemode_request_start_for(pid);
        }
    }
#endif
}

void DirectJavaLaunch::on_state(LoggedProcess::State state)
{
    switch(state)
    {
        case LoggedProcess::FailedToStart:
        {
            //: Error message displayed if instance can't start
            const char *reason = QT_TR_NOOP("Could not launch Minecraft!");
            emit logLine(reason, MessageLevel::Fatal);
            emitFailed(tr(reason));
            return;
        }
        case LoggedProcess::Aborted:
        case LoggedProcess::Crashed:
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->setPid(-1);
            emitFailed(tr("Game crashed."));
            return;
        }
        case LoggedProcess::Finished:
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->setPid(-1);
            // if the exit code wasn't 0, report this as a crash
            auto exitCode = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.exitCode();
            if(exitCode != 0)
            {
                emitFailed(tr("Game crashed."));
                return;
            }
            //FIXME: make this work again
            // hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));
            // run post-exit
            emitSucceeded();
            break;
        }
        case LoggedProcess::Running:
            emit logLine(QString("Minecraft process ID: %1\n\n").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.processId()), MessageLevel::Launcher);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->setPid(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.processId());
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->instance()->setLastLaunch();
            break;
        default:
            break;
    }
}

void DirectJavaLaunch::setWorkingDirectory(const QString &wd)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.setWorkingDirectory(wd);
}

void DirectJavaLaunch::proceed()
{
    // nil
}

bool DirectJavaLaunch::abort()
{
    auto state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.state();
    if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process.kill();
    }
    return true;
}

