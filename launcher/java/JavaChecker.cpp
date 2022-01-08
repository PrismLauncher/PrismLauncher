#include "JavaChecker.h"

#include <QFile>
#include <QProcess>
#include <QMap>
#include <QDebug>

#include "JavaUtils.h"
#include "FileSystem.h"
#include "Commandline.h"
#include "Application.h"

JavaChecker::JavaChecker(QObject *parent) : QObject(parent)
{
}

void JavaChecker::performCheck()
{
    QString checkerJar = FS::PathCombine(APPLICATION->getJarsPath(), "JavaCheck.jar");

    QStringList args;

    process.reset(new QProcess());
    if(m_args.size())
    {
        auto extraArgs = Commandline::splitArgs(m_args);
        args.append(extraArgs);
    }
    if(m_minMem != 0)
    {
        args << QString("-Xms%1m").arg(m_minMem);
    }
    if(m_maxMem != 0)
    {
        args << QString("-Xmx%1m").arg(m_maxMem);
    }
    if(m_permGen != 64)
    {
        args << QString("-XX:PermSize=%1m").arg(m_permGen);
    }

    args.append({"-jar", checkerJar});
    process->setArguments(args);
    process->setProgram(m_path);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->setProcessEnvironment(CleanEnviroment());
    qDebug() << "Running java checker: " + m_path + args.join(" ");;

    connect(process.get(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
    connect(process.get(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(process.get(), SIGNAL(readyReadStandardOutput()), this, SLOT(stdoutReady()));
    connect(process.get(), SIGNAL(readyReadStandardError()), this, SLOT(stderrReady()));
    connect(&killTimer, SIGNAL(timeout()), SLOT(timeout()));
    killTimer.setSingleShot(true);
    killTimer.start(15000);
    process->start();
}

void JavaChecker::stdoutReady()
{
    QByteArray data = process->readAllStandardOutput();
    QString added = QString::fromLocal8Bit(data);
    added.remove('\r');
    m_stdout += added;
}

void JavaChecker::stderrReady()
{
    QByteArray data = process->readAllStandardError();
    QString added = QString::fromLocal8Bit(data);
    added.remove('\r');
    m_stderr += added;
}

void JavaChecker::finished(int exitcode, QProcess::ExitStatus status)
{
    killTimer.stop();
    QProcessPtr _process = process;
    process.reset();

    JavaCheckResult result;
    {
        result.path = m_path;
        result.id = m_id;
    }
    result.errorLog = m_stderr;
    result.outLog = m_stdout;
    qDebug() << "STDOUT" << m_stdout;
    qWarning() << "STDERR" << m_stderr;
    qDebug() << "Java checker finished with status " << status << " exit code " << exitcode;

    if (status == QProcess::CrashExit || exitcode == 1)
    {
        result.validity = JavaCheckResult::Validity::Errored;
        emit checkFinished(result);
        return;
    }

    bool success = true;

    QMap<QString, QString> results;
    QStringList lines = m_stdout.split("\n", QString::SkipEmptyParts);
    for(QString line : lines)
    {
        line = line.trimmed();
        // NOTE: workaround for GH-4125, where garbage is getting printed into stdout on bedrock linux
        if (line.contains("/bedrock/strata")) {
            continue;
        }

        auto parts = line.split('=', QString::SkipEmptyParts);
        if(parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty())
        {
            success = false;
        }
        else
        {
            results.insert(parts[0], parts[1]);
        }
    }

    if(!results.contains("os.arch") || !results.contains("java.version") || !results.contains("java.vendor") || !success)
    {
        result.validity = JavaCheckResult::Validity::ReturnedInvalidData;
        emit checkFinished(result);
        return;
    }

    auto os_arch = results["os.arch"];
    auto java_version = results["java.version"];
    auto java_vendor = results["java.vendor"];
    bool is_64 = os_arch == "x86_64" || os_arch == "amd64";


    result.validity = JavaCheckResult::Validity::Valid;
    result.is_64bit = is_64;
    result.mojangPlatform = is_64 ? "64" : "32";
    result.realPlatform = os_arch;
    result.javaVersion = java_version;
    result.javaVendor = java_vendor;
    qDebug() << "Java checker succeeded.";
    emit checkFinished(result);
}

void JavaChecker::error(QProcess::ProcessError err)
{
    if(err == QProcess::FailedToStart)
    {
        qDebug() << "Java checker has failed to start.";
        qDebug() << "Process environment:";
        qDebug() << process->environment();
        qDebug() << "Native environment:";
        qDebug() << QProcessEnvironment::systemEnvironment().toStringList();
        killTimer.stop();
        JavaCheckResult result;
        {
            result.path = m_path;
            result.id = m_id;
        }

        emit checkFinished(result);
        return;
    }
}

void JavaChecker::timeout()
{
    // NO MERCY. NO ABUSE.
    if(process)
    {
        qDebug() << "Java checker has been killed by timeout.";
        process->kill();
    }
}
