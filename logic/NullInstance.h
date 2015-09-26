#include "BaseInstance.h"

class NullInstance: public BaseInstance
{
public:
	NullInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir)
	:BaseInstance(globalSettings, settings, rootDir)
	{
		setFlag(BaseInstance::VersionBrokenFlag);
	}
	virtual ~NullInstance() {};
	virtual bool setIntendedVersionId(QString) override
	{
		return false;
	}
	virtual void cleanupAfterRun() override
	{
	}
	virtual QString currentVersionId() const override
	{
		return "Null";
	};
	virtual QString intendedVersionId() const override
	{
		return "Null";
	};
	virtual void init() override
	{
	};
	virtual QString getStatusbarDescription() override
	{
		return tr("Unknown instance type");
	};
	virtual bool shouldUpdate() const override
	{
		return false;
	};
	virtual QSet< QString > traits() override
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
	virtual std::shared_ptr< Task > createUpdateTask() override
	{
		return nullptr;
	}
	virtual std::shared_ptr<Task> createJarModdingTask() override
	{
		return nullptr;
	}
	virtual void setShouldUpdate(bool) override
	{
	};
	virtual std::shared_ptr< BaseVersionList > versionList() const override
	{
		return nullptr;
	};
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
};
