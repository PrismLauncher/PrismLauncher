#include "JVisualVM.h"

#include <QDir>
#include <QStandardPaths>

#include "settings/SettingsObject.h"
#include "launch/LaunchTask.h"
#include "BaseInstance.h"

class JVisualVM : public BaseProfiler
{
    Q_OBJECT
public:
    JVisualVM(SettingsObjectPtr settings, InstancePtr instance, QObject *parent = 0);

private slots:
    void profilerStarted();
    void profilerFinished(int exit, QProcess::ExitStatus status);

protected:
    void beginProfilingImpl(shared_qobject_ptr<LaunchTask> process);
};


JVisualVM::JVisualVM(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
    : BaseProfiler(settings, instance, parent)
{
}

void JVisualVM::profilerStarted()
{
    emit readyToLaunch(tr("JVisualVM started"));
}

void JVisualVM::profilerFinished(int exit, QProcess::ExitStatus status)
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

void JVisualVM::beginProfilingImpl(shared_qobject_ptr<LaunchTask> process)
{
    QProcess *profiler = new QProcess(this);
    QStringList profilerArgs =
    {
        "--openpid", QString::number(process->pid())
    };
    auto programPath = globalSettings->get("JVisualVMPath").toString();

    profiler->setArguments(profilerArgs);
    profiler->setProgram(programPath);

    connect(profiler, &QProcess::started, this, &JVisualVM::profilerStarted);
    connect(profiler, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,  &JVisualVM::profilerFinished);

    profiler->start();
    m_profilerProcess = profiler;
}

void JVisualVMFactory::registerSettings(SettingsObjectPtr settings)
{
    QString defaultValue = QStandardPaths::findExecutable("jvisualvm");
    if (defaultValue.isNull())
    {
        defaultValue = QStandardPaths::findExecutable("visualvm");
    }
    settings->registerSetting("JVisualVMPath", defaultValue);
    globalSettings = settings;
}

BaseExternalTool *JVisualVMFactory::createTool(InstancePtr instance, QObject *parent)
{
    return new JVisualVM(globalSettings, instance, parent);
}

bool JVisualVMFactory::check(QString *error)
{
    return check(globalSettings->get("JVisualVMPath").toString(), error);
}

bool JVisualVMFactory::check(const QString &path, QString *error)
{
    if (path.isEmpty())
    {
        *error = QObject::tr("Empty path");
        return false;
    }
    QFileInfo finfo(path);
    if (!finfo.isExecutable() || !finfo.fileName().contains("visualvm"))
    {
        *error = QObject::tr("Invalid path to JVisualVM");
        return false;
    }
    return true;
}

#include "JVisualVM.moc"
