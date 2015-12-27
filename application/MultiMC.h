#pragma once

#include <QApplication>
#include <memory>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QDateTime>
#include <updater/GoUpdate.h>

class GenericPageProvider;
class QFile;
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

class MultiMC : public QApplication
{
	// friends for the purpose of limiting access to deprecated stuff
	friend class MultiMCPage;
	friend class MainWindow;
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

	// InstanceList, IconList, OneSixFTBInstance, LegacyUpdate, LegacyInstance, MCEditTool, JVisualVM, MinecraftInstance, JProfiler, BaseInstance
	std::shared_ptr<SettingsObject> settings()
	{
		return m_settings;
	}

	std::shared_ptr<GenericPageProvider> globalSettingsPages()
	{
		return m_globalSettingsProvider;
	}

	qint64 timeSinceStart() const
	{
		return startTime.msecsTo(QDateTime::currentDateTime());
	}

	QIcon getThemedIcon(const QString& name);

	void setIconTheme(const QString& name);

	// DownloadUpdateTask
	std::shared_ptr<UpdateChecker> updateChecker()
	{
		return m_updateChecker;
	}

	std::shared_ptr<MinecraftVersionList> minecraftlist();
	std::shared_ptr<LWJGLVersionList> lwjgllist();
	std::shared_ptr<ForgeVersionList> forgelist();
	std::shared_ptr<LiteLoaderVersionList> liteloaderlist();
	std::shared_ptr<JavaVersionList> javalist();

	// APPLICATION ONLY
	std::shared_ptr<InstanceList> instances()
	{
		return m_instances;
	}

	// APPLICATION ONLY
	std::shared_ptr<MojangAccountList> accounts()
	{
		return m_accounts;
	}

	// APPLICATION ONLY
	Status status()
	{
		return m_status;
	}

	// APPLICATION ONLY
	QMap<QString, std::shared_ptr<BaseProfilerFactory>> profilers()
	{
		return m_profilers;
	}

	// APPLICATION ONLY
	QMap<QString, std::shared_ptr<BaseDetachedToolFactory>> tools()
	{
		return m_tools;
	}

	// APPLICATION ONLY
	QString getFinishCmd();
	void installUpdates(const QString updateFilesDir, GoUpdate::OperationList operations);
	void updateXP(const QString updateFilesDir, GoUpdate::OperationList operations);
	void updateModern(const QString updateFilesDir, GoUpdate::OperationList operations);

	/*!
	 * Opens a json file using either a system default editor, or, if note empty, the editor
	 * specified in the settings
	 */
	bool openJsonEditor(const QString &filename);

protected: /* to be removed! */
	// FIXME: remove. used by MainWindow to create application update tasks
	/// this is the root of the 'installation'. Used for automatic updates
	const QString &root()
	{
		return rootPath;
	}

private slots:
	/**
	 * Do all the things that should be done before we exit
	 */
	void onExit();

private:
	void initLogger();
	void initIcons();
	void initGlobalSettings(bool test_mode);
	void initTranslations();
	void initSSL();

private:
	friend class UpdateCheckerTest;
	friend class DownloadTaskTest;

	QDateTime startTime;

	std::shared_ptr<QTranslator> m_qt_translator;
	std::shared_ptr<QTranslator> m_mmc_translator;
	std::shared_ptr<SettingsObject> m_settings;
	std::shared_ptr<InstanceList> m_instances;
	std::shared_ptr<UpdateChecker> m_updateChecker;
	std::shared_ptr<MojangAccountList> m_accounts;
	std::shared_ptr<LWJGLVersionList> m_lwjgllist;
	std::shared_ptr<ForgeVersionList> m_forgelist;
	std::shared_ptr<LiteLoaderVersionList> m_liteloaderlist;
	std::shared_ptr<MinecraftVersionList> m_minecraftlist;
	std::shared_ptr<JavaVersionList> m_javalist;
	std::shared_ptr<TranslationDownloader> m_translationChecker;
	std::shared_ptr<GenericPageProvider> m_globalSettingsProvider;

	QMap<QString, std::shared_ptr<BaseProfilerFactory>> m_profilers;
	QMap<QString, std::shared_ptr<BaseDetachedToolFactory>> m_tools;

	QString rootPath;

	Status m_status = MultiMC::Failed;
public:
	QString launchId;
	std::shared_ptr<QFile> logFile;
};
