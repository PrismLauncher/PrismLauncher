#include "JVisualVM.h"

#include <QDir>
#include <QStandardPaths>

#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"

namespace {
class JVisualVM : public BaseProfiler {
    Q_OBJECT
   public:
    JVisualVM(SettingsObject* settings, MinecraftInstance* instance, QObject* parent = nullptr);

   private slots:
    void profilerStarted();
    void profilerFinished(int exit, QProcess::ExitStatus status);

   protected:
    void beginProfilingImpl(LaunchTask* process) override;
};

JVisualVM::JVisualVM(SettingsObject* settings, MinecraftInstance* instance, QObject* parent) : BaseProfiler(settings, instance, parent) {}

void JVisualVM::profilerStarted()
{
    emit readyToLaunch(tr("VisualVM started"));
}

void JVisualVM::profilerFinished([[maybe_unused]] int exit, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        emit abortLaunch(tr("Profiler aborted"));
    }
    if (m_profilerProcess) {
        m_profilerProcess->deleteLater();
        m_profilerProcess = nullptr;
    }
}

void JVisualVM::beginProfilingImpl(LaunchTask* process)
{
    auto* profiler = new QProcess(this);
    QStringList profilerArgs = { "--openpid", QString::number(process->pid()) };
    auto programPath = m_globalSettings->get("JVisualVMPath").toString();

    profiler->setArguments(profilerArgs);
    profiler->setProgram(programPath);

    connect(profiler, &QProcess::started, this, &JVisualVM::profilerStarted);
    connect(profiler, &QProcess::finished, this, &JVisualVM::profilerFinished);

    profiler->start();
    m_profilerProcess = profiler;
}
}  // namespace

void JVisualVMFactory::registerSettings(SettingsObject* settings)
{
    QString defaultValue = QStandardPaths::findExecutable("jvisualvm");
    if (defaultValue.isNull()) {
        defaultValue = QStandardPaths::findExecutable("visualvm");
    }
    settings->registerSetting("JVisualVMPath", defaultValue);
    m_globalSettings = settings;
}

BaseExternalTool* JVisualVMFactory::createTool(MinecraftInstance* instance, QObject* parent)
{
    return new JVisualVM(m_globalSettings, instance, parent);
}

bool JVisualVMFactory::check(QString* error)
{
    return check(m_globalSettings->get("JVisualVMPath").toString(), error);
}

bool JVisualVMFactory::check(const QString& path, QString* error)
{
    if (path.isEmpty()) {
        *error = QObject::tr("Empty path");
        return false;
    }
    QFileInfo finfo(path);
    if (!finfo.isExecutable() || !finfo.fileName().contains("visualvm")) {
        *error = QObject::tr("Invalid path to VisualVM");
        return false;
    }
    return true;
}

#include "JVisualVM.moc"
