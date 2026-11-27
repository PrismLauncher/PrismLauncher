#include "JProfiler.h"

#include <QDir>

#include "Application.h"
#include "BaseInstance.h"
#include "config/GlobalConfig.h"
#include "launch/LaunchTask.h"

class JProfiler : public BaseProfiler {
    Q_OBJECT
   public:
    JProfiler(QObject* parent = 0);

   private slots:
    void profilerStarted();
    void profilerFinished(int exit, QProcess::ExitStatus status);

   protected:
    void beginProfilingImpl(LaunchTask* process);

   private:
    int listeningPort = 0;
};

JProfiler::JProfiler(QObject* parent) : BaseProfiler(parent) {}

void JProfiler::profilerStarted()
{
    emit readyToLaunch(tr("Listening on port: %1").arg(listeningPort));
}

void JProfiler::profilerFinished([[maybe_unused]] int exit, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        emit abortLaunch(tr("Profiler aborted"));
    }
    if (m_profilerProcess) {
        m_profilerProcess->deleteLater();
        m_profilerProcess = 0;
    }
}

void JProfiler::beginProfilingImpl(LaunchTask* process)
{
    listeningPort = APPLICATION->config()->jProfilerPort;
    QProcess* profiler = new QProcess(this);
    QStringList profilerArgs = { "-d", QString::number(process->pid()), "--gui", "-p", QString::number(listeningPort) };
    auto basePath = APPLICATION->config()->jProfilerPath;

#ifdef Q_OS_WIN
    QString profilerProgram = QDir(basePath).absoluteFilePath("bin/jpenable.exe");
#else
    QString profilerProgram = QDir(basePath).absoluteFilePath("bin/jpenable");
#endif

    profiler->setArguments(profilerArgs);
    profiler->setProgram(profilerProgram);

    connect(profiler, &QProcess::started, this, &JProfiler::profilerStarted);
    connect(profiler, &QProcess::finished, this, &JProfiler::profilerFinished);

    m_profilerProcess = profiler;
    profiler->start();
}

BaseExternalTool* JProfilerFactory::createTool(QObject* parent)
{
    return new JProfiler(parent);
}

bool JProfilerFactory::check(QString* error)
{
    return check(APPLICATION->config()->jProfilerPath, error);
}

bool JProfilerFactory::check(const QString& path, QString* error)
{
    if (path.isEmpty()) {
        *error = QObject::tr("Empty path");
        return false;
    }
    QDir dir(path);
    if (!dir.exists()) {
        *error = QObject::tr("Path does not exist");
        return false;
    }
    if (!dir.exists("bin") || !(dir.exists("bin/jprofiler") || dir.exists("bin/jprofiler.exe")) || !dir.exists("bin/agent.jar")) {
        *error = QObject::tr("Invalid JProfiler install");
        return false;
    }
    return true;
}

#include "JProfiler.moc"
