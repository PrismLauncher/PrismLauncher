#pragma once

#include <QApplication>
#include <memory>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QDateTime>
#include <updater/GoUpdate.h>

#include <BaseInstance.h>

class LaunchController;
class LocalPeer;
class InstanceWindow;
class MainWindow;
class SetupWizard;
class FolderInstanceProvider;
class GenericPageProvider;
class QFile;
class LWJGLVersionList;
class HttpMetaCache;
class SettingsObject;
class InstanceList;
class MojangAccountList;
class IconList;
class QNetworkAccessManager;
class JavaInstallList;
class UpdateChecker;
class BaseProfilerFactory;
class BaseDetachedToolFactory;
class TranslationsModel;
class ITheme;
class MCEditTool;
class GAnalytics;

#if defined(MMC)
#undef MMC
#endif
#define MMC (static_cast<MultiMC *>(QCoreApplication::instance()))

class MultiMC : public QApplication
{
	// friends for the purpose of limiting access to deprecated stuff
	Q_OBJECT
public:
	enum Status
	{
		StartingUp,
		UnwritableLog,
		FailedShowError,
		Failed,
		Succeeded,
		Initialized
	};

public:
	MultiMC(int &argc, char **argv);
	virtual ~MultiMC();

	GAnalytics *analytics() const
	{
		return m_analytics;
	}

	std::shared_ptr<SettingsObject> settings() const
	{
		return m_settings;
	}

	std::shared_ptr<GenericPageProvider> globalSettingsPages() const
	{
		return m_globalSettingsProvider;
	}

	qint64 timeSinceStart() const
	{
		return startTime.msecsTo(QDateTime::currentDateTime());
	}

	QIcon getThemedIcon(const QString& name);

	void setIconTheme(const QString& name);

	std::vector<ITheme *> getValidApplicationThemes();

	void setApplicationTheme(const QString& name, bool initial);

	// DownloadUpdateTask
	std::shared_ptr<UpdateChecker> updateChecker()
	{
		return m_updateChecker;
	}

	std::shared_ptr<TranslationsModel> translations();
	std::shared_ptr<LWJGLVersionList> lwjgllist();
	std::shared_ptr<JavaInstallList> javalist();

	std::shared_ptr<InstanceList> instances() const
	{
		return m_instances;
	}

	FolderInstanceProvider * folderProvider() const
	{
		return m_instanceFolder;
	}

	std::shared_ptr<IconList> icons() const
	{
		return m_icons;
	}

	MCEditTool *mcedit() const
	{
		return m_mcedit.get();
	}

	std::shared_ptr<MojangAccountList> accounts() const
	{
		return m_accounts;
	}

	Status status() const
	{
		return m_status;
	}

	const QMap<QString, std::shared_ptr<BaseProfilerFactory>> &profilers() const
	{
		return m_profilers;
	}

	/// this is the root of the 'installation'. Used for automatic updates
	const QString &root()
	{
		return m_rootPath;
	}

	/*!
	 * Opens a json file using either a system default editor, or, if not empty, the editor
	 * specified in the settings
	 */
	bool openJsonEditor(const QString &filename);

	InstanceWindow *showInstanceWindow(InstancePtr instance, QString page = QString());
	MainWindow *showMainWindow(bool minimized = false);

	size_t numRunningInstances()
	{
		return m_runningInstances;
	}

	void updateIsRunning(bool running);
	bool updatesAreAllowed();

signals:
	void updateAllowedChanged(bool status);

public slots:
	bool launch(InstancePtr instance, bool online = true, BaseProfilerFactory *profiler = nullptr);
	bool kill(InstancePtr instance);

private slots:
	/**
	 * Do all the things that should be done before we exit
	 */
	void onExit();
	void on_windowClose();
	void messageReceived(const QString & message);
	void controllerSucceeded();
	void controllerFailed(const QString & error);
	void analyticsSettingChanged(const Setting &setting, QVariant value);
	void setupWizardFinished(int status);

private:
	bool initLogger();
	void shutdownLogger();
	void initIcons();
	void initThemes();
	void initGlobalSettings();
	void initTranslations();
	void initNetwork();
	void initInstances();
	void initAccounts();
	void initMCEdit();
	void initAnalytics();
	void shutdownAnalytics();
	bool createSetupWizard();
	void performMainStartupAction();

	// sets the fatal error message and m_status to Failed.
	void showFatalErrorMessage(const QString & title, const QString & content);

private:
	void addRunningInstance();
	void subRunningInstance();
	bool shouldExitNow() const;

private:
	QDateTime startTime;

	std::shared_ptr<SettingsObject> m_settings;
	std::shared_ptr<InstanceList> m_instances;
	FolderInstanceProvider * m_instanceFolder = nullptr;
	std::shared_ptr<IconList> m_icons;
	std::shared_ptr<UpdateChecker> m_updateChecker;
	std::shared_ptr<MojangAccountList> m_accounts;
	std::shared_ptr<LWJGLVersionList> m_lwjgllist;
	std::shared_ptr<JavaInstallList> m_javalist;
	std::shared_ptr<TranslationsModel> m_translations;
	std::shared_ptr<GenericPageProvider> m_globalSettingsProvider;
	std::map<QString, std::unique_ptr<ITheme>> m_themes;
	std::unique_ptr<MCEditTool> m_mcedit;

	QMap<QString, std::shared_ptr<BaseProfilerFactory>> m_profilers;

	QString m_rootPath;
	Status m_status = MultiMC::StartingUp;

	// used on Windows to attach the standard IO streams
	bool consoleAttached = false;

	// FIXME: attach to instances instead.
	struct InstanceXtras
	{
		InstanceWindow * window = nullptr;
		shared_qobject_ptr<LaunchController> controller;
	};
	std::map<QString, InstanceXtras> m_instanceExtras;

	// main state variables
	size_t m_openWindows = 0;
	size_t m_runningInstances = 0;
	bool m_updateRunning = false;

	// main window, if any
	MainWindow * m_mainWindow = nullptr;

	// peer MultiMC instance connector - used to implement single instance MultiMC and signalling
	LocalPeer * m_peerInstance = nullptr;

	GAnalytics * m_analytics = nullptr;
	SetupWizard * m_setupWizard = nullptr;
public:
	QString m_instanceIdToLaunch;
	bool m_liveCheck = false;
	std::unique_ptr<QFile> logFile;
};
