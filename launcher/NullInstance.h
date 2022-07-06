#pragma once
#include "BaseInstance.h"
#include "launch/LaunchTask.h"

class NullInstance: public BaseInstance
{
    Q_OBJECT
public:
    NullInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir)
    :BaseInstance(globalSettings, settings, rootDir)
    {
        setVersionBroken(true);
    }
    virtual ~NullInstance() {};
    void saveNow() override
    {
    }
    QString getStatusbarDescription() override
    {
        return tr("Unknown instance type");
    };
    QSet< QString > traits() const override
    {
        return {};
    };
    QString instanceConfigFolder() const override
    {
        return instanceRoot();
    };
    shared_qobject_ptr<LaunchTask> createLaunchTask(AuthSessionPtr, MinecraftServerTargetPtr) override
    {
        return nullptr;
    }
    shared_qobject_ptr< Task > createUpdateTask(Net::Mode mode) override
    {
        return nullptr;
    }
    QProcessEnvironment createEnvironment() override
    {
        return QProcessEnvironment();
    }
    QProcessEnvironment createLaunchEnvironment() override
    {
        return QProcessEnvironment();
    }
    QMap<QString, QString> getVariables() const override
    {
        return QMap<QString, QString>();
    }
    IPathMatcher::Ptr getLogFileMatcher() override
    {
        return nullptr;
    }
    QString getLogFileRoot() override
    {
        return instanceRoot();
    }
    QString typeName() const override
    {
        return "Null";
    }
    bool canExport() const override
    {
        return false;
    }
    bool canEdit() const override
    {
        return false;
    }
    bool canLaunch() const override
    {
        return false;
    }
    QStringList verboseDescription(AuthSessionPtr session, MinecraftServerTargetPtr serverToJoin) override
    {
        QStringList out;
        out << "Null instance - placeholder.";
        return out;
    }
    QString modsRoot() const override {
        return QString();
    }
};
