#pragma once

#include <QApplication>
#include <memory>
#include "logger/QsLog.h"
#include "logger/QsLogDest.h"
#include <QFlag>
#include <QIcon>

class MinecraftVersionList;
class LWJGLVersionList;
class HttpMetaCache;
class SettingsObject;
class InstanceList;
class MojangAccountList;
class IconList;
class QNetworkAccessManager;
class ForgeVersionList;
class LiteLoaderVersionList;
class JavaVersionList;
class UpdateChecker;
class BaseProfilerFactory;
class BaseDetachedToolFactory;
class TranslationDownloader;

#if defined(MMC)
#undef MMC
#endif
#define MMC (static_cast<MultiMC *>(QCoreApplication::instance()))

enum UpdateFlag
{
	None = 0x0,
	RestartOnFinish = 0x1,
	DryRun = 0x2,
	OnExit = 0x4
};
Q_DECLARE_FLAGS(UpdateFlags, UpdateFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(UpdateFlags);

class MultiMC : public QApplication
{
	Q_OBJECT
public:
	enum Status
	{
		Failed,
		Succeeded,
		Initialized
	};

public:
	MultiMC(int &argc, char **argv, bool test_mode = false);
	virtual ~MultiMC();

	std::shared_ptr<SettingsObject> settings()
	{
		return m_settings;
	}

	std::shared_ptr<InstanceList> instances()
	{
		return m_instances;
	}

	std::shared_ptr<MojangAccountList> accounts()
	{
		return m_accounts;
	}

	std::shared_ptr<IconList> icons();

	QIcon getThemedIcon(const QString& name);

	void setIconTheme(const QString& name);

	Status status()
	{
		return m_status;
	}

	std::shared_ptr<QNetworkAccessManager> qnam()
	{
		return m_qnam;
	}

	std::shared_ptr<HttpMetaCache> metacache()
	{
		return m_metacache;
	}

	std::shared_ptr<UpdateChecker> updateChecker()
	{
		return m_updateChecker;
	}

	std::shared_ptr<LWJGLVersionList> lwjgllist();

	std::shared_ptr<ForgeVersionList> forgelist();

	std::shared_ptr<LiteLoaderVersionList> liteloaderlist();

	std::shared_ptr<MinecraftVersionList> minecraftlist();

	std::shared_ptr<JavaVersionList> javalist();

	QMap<QString, std::shared_ptr<BaseProfilerFactory>> profilers()
	{
		return m_profilers;
	}
	QMap<QString, std::shared_ptr<BaseDetachedToolFactory>> tools()
	{
		return m_tools;
	}

	void installUpdates(const QString updateFilesDir, UpdateFlags flags = None);

	/*!
	 * Updates the application proxy settings from the settings object.
	 */
	void updateProxySettings();

	/*!
	 * Opens a json file using either a system default editor, or, if note empty, the editor
	 * specified in the settings
	 */
	bool openJsonEditor(const QString &filename);

	/// this is the static data. it stores things that don't move.
	const QString &staticData()
	{
		return staticDataPath;
	}
	/// this is the root of the 'installation'. Used for automatic updates
	const QString &root()
	{
		return rootPath;
	}
	/// this is the where the binary files reside
	const QString &bin()
	{
		return binPath;
	}
	/// this is the work/data path. All user data is here.
	const QString &data()
	{
		return dataPath;
	}
	/**
	 * this is the original work path before it was changed by the adjustment mechanism
	 */
	const QString &origcwd()
	{
		return origcwdPath;
	}

private slots:
	/**
	 * Do all the things that should be done before we exit
	 */
	void onExit();

private:
	void initLogger();

	void initGlobalSettings(bool test_mode);

	void initHttpMetaCache();

	void initTranslations();

private:
	friend class UpdateCheckerTest;
	friend class DownloadUpdateTaskTest;

	std::shared_ptr<QTranslator> m_qt_translator;
	std::shared_ptr<QTranslator> m_mmc_translator;
	std::shared_ptr<SettingsObject> m_settings;
	std::shared_ptr<InstanceList> m_instances;
	std::shared_ptr<UpdateChecker> m_updateChecker;
	std::shared_ptr<MojangAccountList> m_accounts;
	std::shared_ptr<IconList> m_icons;
	std::shared_ptr<QNetworkAccessManager> m_qnam;
	std::shared_ptr<HttpMetaCache> m_metacache;
	std::shared_ptr<LWJGLVersionList> m_lwjgllist;
	std::shared_ptr<ForgeVersionList> m_forgelist;
	std::shared_ptr<LiteLoaderVersionList> m_liteloaderlist;
	std::shared_ptr<MinecraftVersionList> m_minecraftlist;
	std::shared_ptr<JavaVersionList> m_javalist;
	std::shared_ptr<TranslationDownloader> m_translationChecker;

	QMap<QString, std::shared_ptr<BaseProfilerFactory>> m_profilers;
	QMap<QString, std::shared_ptr<BaseDetachedToolFactory>> m_tools;

	QsLogging::DestinationPtr m_fileDestination;
	QsLogging::DestinationPtr m_debugDestination;

	QString m_updateOnExitPath;
	UpdateFlags m_updateOnExitFlags = None;

	QString rootPath;
	QString staticDataPath;
	QString binPath;
	QString dataPath;
	QString origcwdPath;

	Status m_status = MultiMC::Failed;
};
