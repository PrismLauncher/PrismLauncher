#include "JProfiler.h"

#include <QDir>

#include "settings/SettingsObject.h"
#include "launch/LaunchTask.h"
#include "BaseInstance.h"

class JProfiler : public BaseProfiler
{
    Q_OBJECT
public:
    JProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);

private slots:
    void profilerStarted();
    void profilerFinished(int exit, QProcess::ExitStatus status);

protected:
    void beginProfilingImpl(shared_qobject_ptr<LaunchTask> process);

private:
    int listeningPort = 0;
};

JProfiler::JProfiler(SettingsObjectPtr settings, InstancePtr instance,
                     QObject *parent)
    : BaseProfiler(settings, instance, parent)
{
}

void JProfiler::profilerStarted()
{
    emit readyToLaunch(tr("Listening on port: %1").arg(listeningPort));
}

void JProfiler::profilerFinished(int exit, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit)
    {
        emit abortLaunch(tr("Profiler aborted"));
    }
    if (m_profilerProcess)
    {
        m_profilerProcess->deleteLater();
        m_profilerProcess = 0;
    }
}

void JProfiler::beginProfilingImpl(shared_qobject_ptr<LaunchTask> process)
{
    listeningPort = globalSettings->get("JProfilerPort").toInt();
    QProcess *profiler = new QProcess(this);
    QStringList profilerArgs =
    {
        "-d", QString::number(process->pid()),
        "--gui",
        "-p", QString::number(listeningPort)
    };
    auto basePath = globalSettings->get("JProfilerPath").toString();

#ifdef Q_OS_WIN
    QString profilerProgram = QDir(basePath).absoluteFilePath("bin/jpenable.exe");
#else
    QString profilerProgram = QDir(basePath).absoluteFilePath("bin/jpenable");
#endif

    profiler->setArguments(profilerArgs);
    profiler->setProgram(profilerProgram);

    connect(profiler, &QProcess::started, this, &JProfiler::profilerStarted);
    connect(profiler, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &JProfiler::profilerFinished);

    m_profilerProcess = profiler;
    profiler->start();
}

void JProfilerFactory::registerSettings(SettingsObjectPtr settings)
{
    settings->registerSetting("JProfilerPath");
    settings->registerSetting("JProfilerPort", 42042);
    globalSettings = settings;
}

BaseExternalTool *JProfilerFactory::createTool(InstancePtr instance, QObject *parent)
{
    return new JProfiler(globalSettings, instance, parent);
}

bool JProfilerFactory::check(QString *error)
{
    return check(globalSettings->get("JProfilerPath").toString(), error);
}

bool JProfilerFactory::check(const QString &path, QString *error)
{
    if (path.isEmpty())
    {
        *error = QObject::tr("Empty path");
        return false;
    }
    QDir dir(path);
    if (!dir.exists())
    {
        *error = QObject::tr("Path does not exist");
        return false;
    }
    if (!dir.exists("bin") || !(dir.exists("bin/jprofiler") || dir.exists("bin/jprofiler.exe")) || !dir.exists("bin/agent.jar"))
    {
        *error = QObject::tr("Invalid JProfiler install");
        return false;
    }
    return true;
}

#include "JProfiler.moc"
