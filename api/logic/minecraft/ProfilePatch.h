#pragma once

#include <memory>
#include <QList>
#include <QJsonDocument>
#include <QDateTime>
#include "ProblemProvider.h"

class MinecraftProfile;
namespace Meta
{
	class Version;
	class VersionList;
}
class VersionFile;

class ProfilePatch : public ProblemProvider
{
public:
	ProfilePatch(std::shared_ptr<Meta::Version> version);
	ProfilePatch(std::shared_ptr<VersionFile> file, const QString &filename = QString());

	virtual ~ProfilePatch(){};
	virtual void applyTo(MinecraftProfile *profile);

	virtual bool isMoveable();
	virtual bool isCustomizable();
	virtual bool isRevertible();
	virtual bool isRemovable();
	virtual bool isCustom();
	virtual bool isVersionChangeable();

	virtual void setOrder(int order);
	virtual int getOrder();

	virtual QString getID();
	virtual QString getName();
	virtual QString getVersion();
	virtual QDateTime getReleaseDateTime();

	virtual QString getFilename();

	virtual std::shared_ptr<class VersionFile> getVersionFile();
	virtual std::shared_ptr<class Meta::VersionList> getVersionList();

	void setVanilla (bool state);
	void setRemovable (bool state);
	void setRevertible (bool state);
	void setMovable (bool state);


	const QList<PatchProblem> getProblems() override;
	ProblemSeverity getProblemSeverity() override;

protected:
	// Properties for UI and version manipulation from UI in general
	bool m_isMovable = false;
	bool m_isRevertible = false;
	bool m_isRemovable = false;
	bool m_isVanilla = false;

	bool m_orderOverride = false;
	int m_order = 0;

	std::shared_ptr<Meta::Version> m_metaVersion;
	std::shared_ptr<VersionFile> m_file;
	QString m_filename;
};

typedef std::shared_ptr<ProfilePatch> ProfilePatchPtr;
