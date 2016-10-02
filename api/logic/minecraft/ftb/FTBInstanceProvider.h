#pragma once

#include "BaseInstanceProvider.h"
#include <QMap>

class QFileSystemWatcher;

struct MULTIMC_LOGIC_EXPORT FTBRecord
{
	QString dirName;
	QString name;
	QString logo;
	QString iconKey;
	QString mcVersion;
	QString description;
	QString instanceDir;
	QString templateDir;
	bool operator==(const FTBRecord other) const
	{
		return instanceDir == other.instanceDir;
	}
};

class MULTIMC_LOGIC_EXPORT FTBInstanceProvider : public BaseInstanceProvider
{
	Q_OBJECT

public:
	FTBInstanceProvider (SettingsObjectPtr settings);

public:
	QList<InstanceId> discoverInstances() override;
	InstancePtr loadInstance(const InstanceId& id) override;
	void loadGroupList() override {};
	void saveGroupList() override {};

private: /* methods */
	void discoverFTBEntries();
	InstancePtr createInstance(const FTBRecord & record) const;
	InstancePtr loadInstance(const FTBRecord & record) const;


private:
	QMap<InstanceId, FTBRecord> m_records;
};
