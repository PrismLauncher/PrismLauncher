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
	virtual QString currentVersionId() const
	{
		return "Null";
	};
	virtual QString intendedVersionId() const
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
	virtual QSet< QString > traits()
	{
		return {};
	};
	virtual QString instanceConfigFolder() const
	{
		return instanceRoot();
	};
	virtual std::shared_ptr<BaseLauncher> prepareForLaunch(AuthSessionPtr)
	{
		return nullptr;
	}
	virtual std::shared_ptr< Task > doUpdate()
	{
		return nullptr;
	}
	virtual void setShouldUpdate(bool)
	{
	};
	virtual std::shared_ptr< BaseVersionList > versionList() const
	{
		return nullptr;
	};
};
