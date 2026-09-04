#include "JProfiler.h"

#include <QDir>

#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"

namespace {

class JProfiler : public BaseProfiler {
    Q_OBJECT
   public:
    JProfiler(SettingsObject* settings, MinecraftInstance* instance, QObject* parent = nullptr);

   private slots:
    void profilerStarted();
    void profilerFinished(int exit, QProcess::ExitStatus status);

   protected:
    void beginProfilingImpl(LaunchTask* process) override;

   private:
    int m_listeningPort = 0;
};

JProfiler::JProfiler(SettingsObject* settings, MinecraftInstance* instance, QObject* parent) : BaseProfiler(settings, instance, parent) {}

void JProfiler::profilerStarted()
{
    emit readyToLaunch(tr("Listening on port: %1").arg(m_listeningPort));
}

void JProfiler::profilerFinished([[maybe_unused]] int exit, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        emit abortLaunch(tr("Profiler aborted"));
    }
    if (m_profilerProcess) {
        m_profilerProcess->deleteLater();
        m_profilerProcess = nullptr;
    }
}

void JProfiler::beginProfilingImpl(LaunchTask* process)
{
    m_listeningPort = m_globalSettings->get("JProfilerPort").toInt();
    auto* profiler = new QProcess(this);
    QStringList profilerArgs = { "-d", QString::number(process->pid()), "--gui", "-p", QString::number(m_listeningPort) };
    auto basePath = m_globalSettings->get("JProfilerPath").toString();

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
}  // namespace
void JProfilerFactory::registerSettings(SettingsObject* settings)
{
    settings->registerSetting("JProfilerPath");
    settings->registerSetting("JProfilerPort", 42042);
    m_globalSettings = settings;
}

BaseExternalTool* JProfilerFactory::createTool(MinecraftInstance* instance, QObject* parent)
{
    return new JProfiler(m_globalSettings, instance, parent);
}

bool JProfilerFactory::check(QString* error)
{
    return check(m_globalSettings->get("JProfilerPath").toString(), error);
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
