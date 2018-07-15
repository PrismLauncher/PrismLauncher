#pragma once
#include "BaseInstance.h"

class NullInstance: public BaseInstance
{
public:
    NullInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir)
    :BaseInstance(globalSettings, settings, rootDir)
    {
        setVersionBroken(true);
    }
    virtual ~NullInstance() {};
    virtual void init() override
    {
    }
    virtual void saveNow() override
    {
    }
    virtual QString getStatusbarDescription() override
    {
        return tr("Unknown instance type");
    };
    virtual QSet< QString > traits() const override
    {
        return {};
    };
    virtual QString instanceConfigFolder() const override
    {
        return instanceRoot();
    };
    virtual std::shared_ptr<LaunchTask> createLaunchTask(AuthSessionPtr) override
    {
        return nullptr;
    }
    virtual shared_qobject_ptr< Task > createUpdateTask(Net::Mode mode) override
    {
        return nullptr;
    }
    virtual QProcessEnvironment createEnvironment() override
    {
        return QProcessEnvironment();
    }
    virtual QMap<QString, QString> getVariables() const override
    {
        return QMap<QString, QString>();
    }
    virtual IPathMatcher::Ptr getLogFileMatcher() override
    {
        return nullptr;
    }
    virtual QString getLogFileRoot() override
    {
        return instanceRoot();
    }
    virtual QString typeName() const override
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
    QStringList verboseDescription(AuthSessionPtr session) override
    {
        QStringList out;
        out << "Null instance - placeholder.";
        return out;
    }
};
