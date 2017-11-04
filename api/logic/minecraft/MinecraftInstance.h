#pragma once
#include "BaseInstance.h"
#include <java/JavaVersion.h>
#include "minecraft/Mod.h"
#include <QProcess>
#include <QDir>
#include "multimc_logic_export.h"

class ModList;
class WorldList;
class LaunchStep;
class ComponentList;

class MULTIMC_LOGIC_EXPORT MinecraftInstance: public BaseInstance
{
	Q_OBJECT
public:
	MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual ~MinecraftInstance() {};
	virtual void init() override;

	// FIXME: remove
	QString typeName() const override;
	// FIXME: remove
	QSet<QString> traits() const override;

	bool canEdit() const override
	{
		return true;
	}

	bool canExport() const override
	{
		return true;
	}

	////// Directories and files //////
	QString jarModsDir() const;
	QString resourcePacksDir() const;
	QString texturePacksDir() const;
	QString loaderModsDir() const;
	QString coreModsDir() const;
	QString libDir() const;
	QString worldDir() const;
	QDir jarmodsPath() const;
	QDir librariesPath() const;
	QDir versionsPath() const;
	QString instanceConfigFolder() const override;
	QString minecraftRoot() const; // Path to the instance's minecraft directory.
	QString binRoot() const; // Path to the instance's minecraft bin directory.
	QString getNativePath() const; // where to put the natives during/before launch
	QString getLocalLibraryPath() const; // where the instance-local libraries should be


	//////  Profile management //////
	void createProfile();
	std::shared_ptr<ComponentList> getComponentList() const;
	void reloadProfile();
	void clearProfile();
	bool reload() override;


	//////  Mod Lists  //////
	std::shared_ptr<ModList> loaderModList() const;
	std::shared_ptr<ModList> coreModList() const;
	std::shared_ptr<ModList> resourcePackList() const;
	std::shared_ptr<ModList> texturePackList() const;
	std::shared_ptr<WorldList> worldList() const;


	//////  Launch stuff //////
	shared_qobject_ptr<Task> createUpdateTask() override;
	std::shared_ptr<LaunchTask> createLaunchTask(AuthSessionPtr account) override;
	QStringList extraArguments() const override;
	QStringList verboseDescription(AuthSessionPtr session) override;
	QList<Mod> getJarMods() const;
	QString createLaunchScript(AuthSessionPtr session);
	/// get arguments passed to java
	QStringList javaArguments() const;

	/// get variables for launch command variable substitution/environment
	QMap<QString, QString> getVariables() const override;

	/// create an environment for launching processes
	QProcessEnvironment createEnvironment() override;

	/// guess log level from a line of minecraft log
	MessageLevel::Enum guessLevel(const QString &line, MessageLevel::Enum level) override;

	IPathMatcher::Ptr getLogFileMatcher() override;

	QString getLogFileRoot() override;

	QString getStatusbarDescription() override;

	// FIXME: remove
	virtual QStringList getClassPath() const;
	// FIXME: remove
	virtual QStringList getNativeJars() const;
	// FIXME: remove
	virtual QString getMainClass() const;

	// FIXME: remove
	virtual QStringList processMinecraftArgs(AuthSessionPtr account) const;

	virtual JavaVersion getJavaVersion() const;

	// FIXME: remove
	QString getComponentVersion(const QString &uid) const;
	// FIXME: remove
	bool setComponentVersion(const QString &uid, const QString &version);

signals:
	void versionReloaded();

protected:
	QMap<QString, QString> createCensorFilterFromSession(AuthSessionPtr session);
	QStringList validLaunchMethods();
	QString launchMethod();

private:
	QString prettifyTimeDuration(int64_t duration);

protected: // data
	std::shared_ptr<ComponentList> m_components;
	mutable std::shared_ptr<ModList> m_loader_mod_list;
	mutable std::shared_ptr<ModList> m_core_mod_list;
	mutable std::shared_ptr<ModList> m_resource_pack_list;
	mutable std::shared_ptr<ModList> m_texture_pack_list;
	mutable std::shared_ptr<WorldList> m_world_list;
};

typedef std::shared_ptr<MinecraftInstance> MinecraftInstancePtr;
