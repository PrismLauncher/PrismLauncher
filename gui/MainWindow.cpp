/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "MultiMC.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>

#include <QDesktopServices>
#include <QKeyEvent>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QToolButton>
#include <QWidgetAction>
#include <QProgressDialog>

#include "osutils.h"
#include "userutils.h"
#include "pathutils.h"

#include "gui/groupview/GroupView.h"
#include "gui/groupview/InstanceDelegate.h"

#include "gui/Platform.h"

#include "gui/widgets/LabeledToolButton.h"

#include "gui/dialogs/SettingsDialog.h"
#include "gui/dialogs/NewInstanceDialog.h"
#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/LwjglSelectDialog.h"
#include "gui/dialogs/InstanceSettings.h"
#include "gui/dialogs/IconPickerDialog.h"
#include "gui/dialogs/EditNotesDialog.h"
#include "gui/dialogs/CopyInstanceDialog.h"
#include "gui/dialogs/AccountListDialog.h"
#include "gui/dialogs/AccountSelectDialog.h"
#include "gui/dialogs/UpdateDialog.h"
#include "gui/dialogs/EditAccountDialog.h"
#include "gui/dialogs/ScreenshotDialog.h"

#include "gui/ConsoleWindow.h"

#include "logic/lists/InstanceList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/icons/IconList.h"
#include "logic/lists/JavaVersionList.h"

#include "logic/auth/flows/AuthenticateTask.h"
#include "logic/auth/flows/RefreshTask.h"

#include "logic/updater/DownloadUpdateTask.h"

#include "logic/news/NewsChecker.h"

#include "logic/status/StatusChecker.h"

#include "logic/net/URLConstants.h"
#include "logic/net/NetJob.h"

#include "logic/BaseInstance.h"
#include "logic/InstanceFactory.h"
#include "logic/MinecraftProcess.h"
#include "logic/OneSixUpdate.h"
#include "logic/JavaUtils.h"
#include "logic/NagUtils.h"
#include "logic/SkinUtils.h"

#include "logic/LegacyInstance.h"

#include "logic/assets/AssetsUtils.h"
#include "logic/assets/AssetsMigrateTask.h"
#include <logic/updater/UpdateChecker.h>
#include <logic/updater/NotificationChecker.h>
#include <logic/tasks/ThreadTask.h>

#include "logic/tools/BaseProfiler.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	QString winTitle = QString("MultiMC 5 - Version %1").arg(MMC->version().toString());
	if (!MMC->version().platform.isEmpty())
		winTitle += " on " + MMC->version().platform;
	setWindowTitle(winTitle);

	// OSX magic.
	// setUnifiedTitleAndToolBarOnMac(true);

	// The instance action toolbar customizations
	{
		// disabled until we have an instance selected
		ui->instanceToolBar->setEnabled(false);

		// the rename label is inside the rename tool button
		renameButton = new LabeledToolButton();
		renameButton->setText("Instance Name");
		renameButton->setToolTip(ui->actionRenameInstance->toolTip());
		connect(renameButton, SIGNAL(clicked(bool)), SLOT(on_actionRenameInstance_triggered()));
		ui->instanceToolBar->insertWidget(ui->actionLaunchInstance, renameButton);
		ui->instanceToolBar->insertSeparator(ui->actionLaunchInstance);
		renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	}

	// Add the news label to the news toolbar.
	{
		newsLabel = new QToolButton();
		newsLabel->setIcon(QIcon::fromTheme("news"));
		newsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		newsLabel->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		ui->newsToolBar->insertWidget(ui->actionMoreNews, newsLabel);
		QObject::connect(newsLabel, &QAbstractButton::clicked, this,
						 &MainWindow::newsButtonClicked);
		QObject::connect(MMC->newsChecker().get(), &NewsChecker::newsLoaded, this,
						 &MainWindow::updateNewsLabel);
		updateNewsLabel();
	}

	// Create the instance list widget
	{
		view = new GroupView(ui->centralWidget);

		view->setSelectionMode(QAbstractItemView::SingleSelection);
		// view->setCategoryDrawer(drawer);
		// view->setCollapsibleBlocks(true);
		// view->setViewMode(QListView::IconMode);
		// view->setFlow(QListView::LeftToRight);
		// view->setWordWrap(true);
		// view->setMouseTracking(true);
		// view->viewport()->setAttribute(Qt::WA_Hover);
		auto delegate = new ListViewDelegate();
		view->setItemDelegate(delegate);
		// view->setSpacing(10);
		// view->setUniformItemWidths(true);

		// do not show ugly blue border on the mac
		view->setAttribute(Qt::WA_MacShowFocusRect, false);

		view->installEventFilter(this);

		proxymodel = new InstanceProxyModel(this);
		//		proxymodel->setSortRole(KCategorizedSortFilterProxyModel::CategorySortRole);
		// proxymodel->setFilterRole(KCategorizedSortFilterProxyModel::CategorySortRole);
		// proxymodel->setDynamicSortFilter ( true );

		// FIXME: instList should be global-ish, or at least not tied to the main window...
		// maybe the application itself?
		proxymodel->setSourceModel(MMC->instances().get());
		proxymodel->sort(0);
		view->setFrameShape(QFrame::NoFrame);
		view->setModel(proxymodel);

		//view->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(view, SIGNAL(customContextMenuRequested(const QPoint &)), this,
				SLOT(showInstanceContextMenu(const QPoint &)));

		ui->horizontalLayout->addWidget(view);
	}
	// The cat background
	{
		bool cat_enable = MMC->settings()->get("TheCat").toBool();
		ui->actionCAT->setChecked(cat_enable);
		connect(ui->actionCAT, SIGNAL(toggled(bool)), SLOT(onCatToggled(bool)));
		setCatBackground(cat_enable);
	}
	// start instance when double-clicked
	connect(view, SIGNAL(doubleClicked(const QModelIndex &)), this,
			SLOT(instanceActivated(const QModelIndex &)));
	// track the selection -- update the instance toolbar
	connect(view->selectionModel(),
			SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
			SLOT(instanceChanged(const QModelIndex &, const QModelIndex &)));

	// track icon changes and update the toolbar!
	connect(MMC->icons().get(), SIGNAL(iconUpdated(QString)), SLOT(iconUpdated(QString)));

	// model reset -> selection is invalid. All the instance pointers are wrong.
	// FIXME: stop using POINTERS everywhere
	connect(MMC->instances().get(), SIGNAL(dataIsInvalid()), SLOT(selectionBad()));

	m_statusLeft = new QLabel(tr("No instance selected"), this);
	m_statusRight = new QLabel(tr("No status available"), this);
	m_statusRefresh = new QToolButton(this);
	m_statusRefresh->setCheckable(true);
	m_statusRefresh->setToolButtonStyle(Qt::ToolButtonIconOnly);
	m_statusRefresh->setIcon(QIcon::fromTheme("refresh"));

	statusBar()->addPermanentWidget(m_statusLeft, 1);
	statusBar()->addPermanentWidget(m_statusRight, 0);
	statusBar()->addPermanentWidget(m_statusRefresh, 0);

	// Start status checker
	{
		connect(MMC->statusChecker().get(), &StatusChecker::statusLoaded, this,
				&MainWindow::updateStatusUI);
		connect(MMC->statusChecker().get(), &StatusChecker::statusLoadingFailed, this,
				&MainWindow::updateStatusFailedUI);

		connect(m_statusRefresh, &QAbstractButton::clicked, this, &MainWindow::reloadStatus);
		connect(&statusTimer, &QTimer::timeout, this, &MainWindow::reloadStatus);
		statusTimer.setSingleShot(true);

		reloadStatus();
	}

	// Add "manage accounts" button, right align
	QWidget *spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui->mainToolBar->addWidget(spacer);

	accountMenu = new QMenu(this);
	manageAccountsAction = new QAction(tr("Manage Accounts"), this);
	manageAccountsAction->setCheckable(false);
	connect(manageAccountsAction, SIGNAL(triggered(bool)), this,
			SLOT(on_actionManageAccounts_triggered()));

	repopulateAccountsMenu();

	accountMenuButton = new QToolButton(this);
	accountMenuButton->setText(tr("Accounts"));
	accountMenuButton->setMenu(accountMenu);
	accountMenuButton->setPopupMode(QToolButton::InstantPopup);
	accountMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	accountMenuButton->setIcon(QIcon::fromTheme("noaccount"));

	QWidgetAction *accountMenuButtonAction = new QWidgetAction(this);
	accountMenuButtonAction->setDefaultWidget(accountMenuButton);

	ui->mainToolBar->addAction(accountMenuButtonAction);

	// Update the menu when the active account changes.
	// Shouldn't have to use lambdas here like this, but if I don't, the compiler throws a fit.
	// Template hell sucks...
	connect(MMC->accounts().get(), &MojangAccountList::activeAccountChanged, [this]
	{ activeAccountChanged(); });
	connect(MMC->accounts().get(), &MojangAccountList::listChanged, [this]
	{ repopulateAccountsMenu(); });

	// Show initial account
	activeAccountChanged();

	auto accounts = MMC->accounts();

	// TODO: Nicer way to iterate?
	for (int i = 0; i < accounts->count(); i++)
	{
		auto account = accounts->at(i);
		if (account != nullptr)
		{
			auto job = new NetJob("Startup player skins: " + account->username());

			for (auto profile : account->profiles())
			{
				auto meta = MMC->metacache()->resolveEntry("skins", profile.name + ".png");
				auto action = CacheDownload::make(
					QUrl("http://" + URLConstants::SKINS_BASE + profile.name + ".png"), meta);
				job->addNetAction(action);
				meta->stale = true;
			}

			connect(job, SIGNAL(succeeded()), SLOT(activeAccountChanged()));
			job->start();
		}
	}

	// run the things that load and download other things... FIXME: this is NOT the place
	// FIXME: invisible actions in the background = NOPE.
	{
		if (!MMC->minecraftlist()->isLoaded())
		{
			m_versionLoadTask = MMC->minecraftlist()->getLoadTask();
			startTask(m_versionLoadTask);
		}
		if (!MMC->lwjgllist()->isLoaded())
		{
			MMC->lwjgllist()->loadList();
		}

		MMC->newsChecker()->reloadNews();
		updateNewsLabel();

		// set up the updater object.
		auto updater = MMC->updateChecker();
		connect(updater.get(), &UpdateChecker::updateAvailable, this,
				&MainWindow::updateAvailable);
		connect(updater.get(), &UpdateChecker::noUpdateFound, [this]()
		{
			CustomMessageBox::selectable(
				this, tr("No update found."),
				tr("No MultiMC update was found!\nYou are using the latest version."))->exec();
		});
		// if automatic update checks are allowed, start one.
		if (MMC->settings()->get("AutoUpdate").toBool())
			on_actionCheckUpdate_triggered();

		connect(MMC->notificationChecker().get(),
				&NotificationChecker::notificationCheckFinished, this,
				&MainWindow::notificationsChanged);
	}

	setSelectedInstanceById(MMC->settings()->get("SelectedInstance").toString());

	// removing this looks stupid
	view->setFocus();
}

MainWindow::~MainWindow()
{
	delete ui;
	delete proxymodel;
}

void MainWindow::showInstanceContextMenu(const QPoint &pos)
{
	if (!view->indexAt(pos).isValid())
	{
		return;
	}

	QList<QAction *> actions = ui->instanceToolBar->actions();

	// HACK: Filthy rename button hack because the instance view is getting rewritten anyway
	QAction *actionRename;
	actionRename = new QAction(tr("Rename"), this);
	actionRename->setToolTip(ui->actionRenameInstance->toolTip());

	connect(actionRename, SIGNAL(triggered(bool)), SLOT(on_actionRenameInstance_triggered()));

	actions.replace(1, actionRename);

	QMenu myMenu;
	myMenu.addActions(actions);
	myMenu.setEnabled(m_selectedInstance->canLaunch());
	myMenu.exec(view->mapToGlobal(pos));
}

void MainWindow::updateToolsMenu()
{
	if (ui->actionLaunchInstance->menu())
	{
		ui->actionLaunchInstance->menu()->deleteLater();
	}
	QMenu *launchMenu = new QMenu(this);
	QAction *normalLaunch = launchMenu->addAction(tr("Launch"));
	connect(normalLaunch, &QAction::triggered, [this](){doLaunch();});
	launchMenu->addSeparator()->setText(tr("Profilers"));
	for (auto profiler : MMC->profilers().values())
	{
		QAction *profilerAction = launchMenu->addAction(profiler->name());
		QString error;
		if (!profiler->check(&error))
		{
			profilerAction->setDisabled(true);
			profilerAction->setToolTip(tr("Profiler not setup correctly. Go into settings, \"External Tools\"."));
		}
		else
		{
			connect(profilerAction, &QAction::triggered, [this, profiler](){doLaunch(true, profiler.get());});
		}
	}
	launchMenu->addSeparator()->setText(tr("Tools"));
	for (auto tool : MMC->tools().values())
	{
		QAction *toolAction = launchMenu->addAction(tool->name());
		QString error;
		if (!tool->check(&error))
		{
			toolAction->setDisabled(true);
			toolAction->setToolTip(tr("Tool not setup correctly. Go into settings, \"External Tools\"."));
		}
		else
		{
			connect(toolAction, &QAction::triggered, [this, tool]()
			{
				tool->createDetachedTool(m_selectedInstance, this)->run();
			});
		}
	}
	ui->actionLaunchInstance->setMenu(launchMenu);
}

void MainWindow::repopulateAccountsMenu()
{
	accountMenu->clear();

	std::shared_ptr<MojangAccountList> accounts = MMC->accounts();
	MojangAccountPtr active_account = accounts->activeAccount();

	QString active_username = "";
	if (active_account != nullptr)
	{
		active_username = accounts->activeAccount()->username();
	}

	if (accounts->count() <= 0)
	{
		QAction *action = new QAction(tr("No accounts added!"), this);
		action->setEnabled(false);
		accountMenu->addAction(action);

		accountMenu->addSeparator();
	}
	else
	{
		// TODO: Nicer way to iterate?
		for (int i = 0; i < accounts->count(); i++)
		{
			MojangAccountPtr account = accounts->at(i);

			// Styling hack
			QAction *section = new QAction(account->username(), this);
			section->setEnabled(false);
			accountMenu->addAction(section);

			for (auto profile : account->profiles())
			{
				QAction *action = new QAction(profile.name, this);
				action->setData(account->username());
				action->setCheckable(true);
				if (active_username == account->username())
				{
					action->setChecked(true);
				}

				action->setIcon(SkinUtils::getFaceFromCache(profile.name));
				accountMenu->addAction(action);
				connect(action, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));
			}

			accountMenu->addSeparator();
		}
	}

	QAction *action = new QAction(tr("No Default Account"), this);
	action->setCheckable(true);
	action->setIcon(QIcon::fromTheme("noaccount"));
	action->setData("");
	if (active_username.isEmpty())
	{
		action->setChecked(true);
	}

	accountMenu->addAction(action);
	connect(action, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));

	accountMenu->addSeparator();
	accountMenu->addAction(manageAccountsAction);
}

/*
 * Assumes the sender is a QAction
 */
void MainWindow::changeActiveAccount()
{
	QAction *sAction = (QAction *)sender();
	// Profile's associated Mojang username
	// Will need to change when profiles are properly implemented
	if (sAction->data().type() != QVariant::Type::String)
		return;

	QVariant data = sAction->data();
	QString id = "";
	if (!data.isNull())
	{
		id = data.toString();
	}

	MMC->accounts()->setActiveAccount(id);

	activeAccountChanged();
}

void MainWindow::activeAccountChanged()
{
	repopulateAccountsMenu();

	MojangAccountPtr account = MMC->accounts()->activeAccount();

	if (account != nullptr && account->username() != "")
	{
		const AccountProfile *profile = account->currentProfile();
		if (profile != nullptr)
		{
			accountMenuButton->setIcon(SkinUtils::getFaceFromCache(profile->name));
			return;
		}
	}

	// Set the icon to the "no account" icon.
	accountMenuButton->setIcon(QIcon::fromTheme("noaccount"));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
	if (obj == view)
	{
		if (ev->type() == QEvent::KeyPress)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
			switch (keyEvent->key())
			{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				on_actionLaunchInstance_triggered();
				return true;
			case Qt::Key_Delete:
				on_actionDeleteInstance_triggered();
				return true;
			case Qt::Key_F5:
				on_actionRefresh_triggered();
				return true;
			case Qt::Key_F2:
				on_actionRenameInstance_triggered();
				return true;
			default:
				break;
			}
		}
	}
	return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::updateNewsLabel()
{
	auto newsChecker = MMC->newsChecker();
	if (newsChecker->isLoadingNews())
	{
		newsLabel->setText(tr("Loading news..."));
		newsLabel->setEnabled(false);
	}
	else
	{
		QList<NewsEntryPtr> entries = newsChecker->getNewsEntries();
		if (entries.length() > 0)
		{
			newsLabel->setText(entries[0]->title);
			newsLabel->setEnabled(true);
		}
		else
		{
			newsLabel->setText(tr("No news available."));
			newsLabel->setEnabled(false);
		}
	}
}

static QString convertStatus(const QString &status)
{
	QString ret = "?";

	if (status == "green")
		ret = "↑";
	else if (status == "yellow")
		ret = "-";
	else if (status == "red")
		ret = "↓";

	return "<span style=\"font-size:11pt; font-weight:600;\">" + ret + "</span>";
}

void MainWindow::reloadStatus()
{
	m_statusRefresh->setChecked(true);
	MMC->statusChecker()->reloadStatus();
	// updateStatusUI();
}

static QString makeStatusString(const QMap<QString, QString> statuses)
{
	QString status = "";
	status += "Web: " + convertStatus(statuses["minecraft.net"]);
	status += "  Account: " + convertStatus(statuses["account.mojang.com"]);
	status += "  Skins: " + convertStatus(statuses["skins.minecraft.net"]);
	status += "  Auth: " + convertStatus(statuses["authserver.mojang.com"]);
	status += "  Session: " + convertStatus(statuses["sessionserver.mojang.com"]);

	return status;
}

void MainWindow::updateStatusUI()
{
	auto statusChecker = MMC->statusChecker();
	auto statuses = statusChecker->getStatusEntries();

	QString status = makeStatusString(statuses);
	m_statusRefresh->setChecked(false);

	m_statusRight->setText(status);

	statusTimer.start(60 * 1000);
}

void MainWindow::updateStatusFailedUI()
{
	m_statusRight->setText(makeStatusString(QMap<QString, QString>()));
	m_statusRefresh->setChecked(false);

	statusTimer.start(60 * 1000);
}

void MainWindow::updateAvailable(QString repo, QString versionName, int versionId)
{
	UpdateDialog dlg;
	UpdateAction action = (UpdateAction)dlg.exec();
	switch (action)
	{
	case UPDATE_LATER:
		QLOG_INFO() << "Update will be installed later.";
		break;
	case UPDATE_NOW:
		downloadUpdates(repo, versionId);
		break;
	case UPDATE_ONEXIT:
		downloadUpdates(repo, versionId, true);
		break;
	}
}

QList<int> stringToIntList(const QString &string)
{
	QStringList split = string.split(',', QString::SkipEmptyParts);
	QList<int> out;
	for (int i = 0; i < split.size(); ++i)
	{
		out.append(split.at(i).toInt());
	}
	return out;
}
QString intListToString(const QList<int> &list)
{
	QStringList slist;
	for (int i = 0; i < list.size(); ++i)
	{
		slist.append(QString::number(list.at(i)));
	}
	return slist.join(',');
}
void MainWindow::notificationsChanged()
{
	QList<NotificationChecker::NotificationEntry> entries =
		MMC->notificationChecker()->notificationEntries();
	QList<int> shownNotifications =
		stringToIntList(MMC->settings()->get("ShownNotifications").toString());
	for (auto it = entries.begin(); it != entries.end(); ++it)
	{
		NotificationChecker::NotificationEntry entry = *it;
		if (!shownNotifications.contains(entry.id) && entry.applies())
		{
			QMessageBox::Icon icon;
			switch (entry.type)
			{
			case NotificationChecker::NotificationEntry::Critical:
				icon = QMessageBox::Critical;
				break;
			case NotificationChecker::NotificationEntry::Warning:
				icon = QMessageBox::Warning;
				break;
			case NotificationChecker::NotificationEntry::Information:
				icon = QMessageBox::Information;
				break;
			}

			QMessageBox box(icon, tr("Notification"), entry.message, QMessageBox::Close, this);
			QPushButton *dontShowAgainButton =
				box.addButton(tr("Don't show again"), QMessageBox::AcceptRole);
			box.setDefaultButton(QMessageBox::Close);
			box.exec();
			if (box.clickedButton() == dontShowAgainButton)
			{
				shownNotifications.append(entry.id);
			}
		}
	}
	MMC->settings()->set("ShownNotifications", intListToString(shownNotifications));
}

void MainWindow::downloadUpdates(QString repo, int versionId, bool installOnExit)
{
	QLOG_INFO() << "Downloading updates.";
	// TODO: If the user chooses to update on exit, we should download updates in the
	// background.
	// Doing so is a bit complicated, because we'd have to make sure it finished downloading
	// before actually exiting MultiMC.
	ProgressDialog updateDlg(this);
	DownloadUpdateTask updateTask(repo, versionId, &updateDlg);
	// If the task succeeds, install the updates.
	if (updateDlg.exec(&updateTask))
	{
		UpdateFlags baseFlags = None;
#ifdef MultiMC_UPDATER_DRY_RUN
		baseFlags |= DryRun;
#endif
		if (installOnExit)
			MMC->installUpdates(updateTask.updateFilesDir(), baseFlags | OnExit);
		else
			MMC->installUpdates(updateTask.updateFilesDir(), baseFlags | RestartOnFinish);
	}
}

void MainWindow::onCatToggled(bool state)
{
	setCatBackground(state);
	MMC->settings()->set("TheCat", state);
}

void MainWindow::setCatBackground(bool enabled)
{
	if (enabled)
	{
		view->setStyleSheet("GroupView"
							"{"
							"background-image: url(:/backgrounds/kitteh);"
							"background-attachment: fixed;"
							"background-clip: padding;"
							"background-position: top right;"
							"background-repeat: none;"
							"background-color:palette(base);"
							"}");
	}
	else
	{
		view->setStyleSheet(QString());
	}
}

void MainWindow::on_actionAddInstance_triggered()
{
	if (!MMC->minecraftlist()->isLoaded() && m_versionLoadTask &&
		m_versionLoadTask->isRunning())
	{
		QEventLoop waitLoop;
		waitLoop.connect(m_versionLoadTask, SIGNAL(failed(QString)), SLOT(quit()));
		waitLoop.connect(m_versionLoadTask, SIGNAL(succeeded()), SLOT(quit()));
		waitLoop.exec();
	}

	NewInstanceDialog newInstDlg(this);
	if (!newInstDlg.exec())
		return;

	BaseInstance *newInstance = NULL;

	QString instancesDir = MMC->settings()->get("InstanceDir").toString();
	QString instDirName = DirNameFromString(newInstDlg.instName(), instancesDir);
	QString instDir = PathCombine(instancesDir, instDirName);

	auto &loader = InstanceFactory::get();

	auto error = loader.createInstance(newInstance, newInstDlg.selectedVersion(), instDir);
	QString errorMsg = QString("Failed to create instance %1: ").arg(instDirName);
	switch (error)
	{
	case InstanceFactory::NoCreateError:
		newInstance->setName(newInstDlg.instName());
		newInstance->setIconKey(newInstDlg.iconKey());
		MMC->instances()->add(InstancePtr(newInstance));
		break;

	case InstanceFactory::InstExists:
	{
		errorMsg += "An instance with the given directory name already exists.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		return;
	}

	case InstanceFactory::CantCreateDir:
	{
		errorMsg += "Failed to create the instance directory.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		return;
	}

	default:
	{
		errorMsg += QString("Unknown instance loader error %1").arg(error);
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		return;
	}
	}

	if (MMC->accounts()->anyAccountIsValid())
	{
		ProgressDialog loadDialog(this);
		auto update = newInstance->doUpdate();
		connect(update.get(), &Task::failed, [this](QString reason)
		{
			QString error = QString("Instance load failed: %1").arg(reason);
			CustomMessageBox::selectable(this, tr("Error"), error, QMessageBox::Warning)
				->show();
		});
		loadDialog.exec(update.get());
	}
	else
	{
		CustomMessageBox::selectable(
			this, tr("Error"),
			tr("MultiMC cannot download Minecraft or update instances unless you have at least "
			   "one account added.\nPlease add your Mojang or Minecraft account."),
			QMessageBox::Warning)->show();
	}
}

void MainWindow::on_actionCopyInstance_triggered()
{
	if (!m_selectedInstance)
		return;

	CopyInstanceDialog copyInstDlg(m_selectedInstance, this);
	if (!copyInstDlg.exec())
		return;

	QString instancesDir = MMC->settings()->get("InstanceDir").toString();
	QString instDirName = DirNameFromString(copyInstDlg.instName(), instancesDir);
	QString instDir = PathCombine(instancesDir, instDirName);

	auto &loader = InstanceFactory::get();

	BaseInstance *newInstance = NULL;
	auto error = loader.copyInstance(newInstance, m_selectedInstance, instDir);

	QString errorMsg = QString("Failed to create instance %1: ").arg(instDirName);
	switch (error)
	{
	case InstanceFactory::NoCreateError:
		newInstance->setName(copyInstDlg.instName());
		newInstance->setIconKey(copyInstDlg.iconKey());
		MMC->instances()->add(InstancePtr(newInstance));
		return;

	case InstanceFactory::InstExists:
	{
		errorMsg += "An instance with the given directory name already exists.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}

	case InstanceFactory::CantCreateDir:
	{
		errorMsg += "Failed to create the instance directory.";
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}

	default:
	{
		errorMsg += QString("Unknown instance loader error %1").arg(error);
		CustomMessageBox::selectable(this, tr("Error"), errorMsg, QMessageBox::Warning)->show();
		break;
	}
	}
}

void MainWindow::on_actionChangeInstIcon_triggered()
{
	if (!m_selectedInstance)
		return;

	IconPickerDialog dlg(this);
	dlg.exec(m_selectedInstance->iconKey());
	if (dlg.result() == QDialog::Accepted)
	{
		m_selectedInstance->setIconKey(dlg.selectedIconKey);
		auto ico = MMC->icons()->getBigIcon(dlg.selectedIconKey);
		ui->actionChangeInstIcon->setIcon(ico);
	}
}

void MainWindow::iconUpdated(QString icon)
{
	if (icon == m_currentInstIcon)
	{
		ui->actionChangeInstIcon->setIcon(MMC->icons()->getBigIcon(m_currentInstIcon));
	}
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
	m_currentInstIcon = new_icon;
	ui->actionChangeInstIcon->setIcon(MMC->icons()->getBigIcon(m_currentInstIcon));
}

void MainWindow::setSelectedInstanceById(const QString &id)
{
	QModelIndex selectionIndex = proxymodel->index(0, 0);
	if (!id.isNull())
	{
		const QModelIndex index = MMC->instances()->getInstanceIndexById(id);
		if (index.isValid())
		{
			selectionIndex = proxymodel->mapFromSource(index);
		}
	}
	view->selectionModel()->setCurrentIndex(selectionIndex,
											QItemSelectionModel::ClearAndSelect);
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
	if (!m_selectedInstance)
		return;

	bool ok = false;
	QString name(m_selectedInstance->group());
	auto groups = MMC->instances()->getGroups();
	groups.insert(0, "");
	groups.sort(Qt::CaseInsensitive);
	int foo = groups.indexOf(name);

	name = QInputDialog::getItem(this, tr("Group name"), tr("Enter a new group name."), groups,
								 foo, true, &ok);
	name = name.simplified();
	if (ok)
		m_selectedInstance->setGroupPost(name);
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
	QString str = MMC->settings()->get("InstanceDir").toString();
	openDirInDefaultProgram(str);
}

void MainWindow::on_actionRefresh_triggered()
{
	MMC->instances()->loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
	openDirInDefaultProgram(MMC->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::on_actionConfig_Folder_triggered()
{
	if (m_selectedInstance)
	{
		QString str = m_selectedInstance->instanceConfigFolder();
		openDirInDefaultProgram(QDir(str).absolutePath());
	}
}

void MainWindow::on_actionCheckUpdate_triggered()
{
	auto updater = MMC->updateChecker();

	updater->checkForUpdate(true);
}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog dialog(this);
	dialog.exec();
	// FIXME: quick HACK to make this work. improve, optimize.
	proxymodel->invalidate();
	proxymodel->sort(0);
	updateToolsMenu();
}

void MainWindow::on_actionManageAccounts_triggered()
{
	AccountListDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_actionReportBug_triggered()
{
	openWebPage(QUrl("https://github.com/MultiMC/MultiMC5/issues"));
}

void MainWindow::on_actionMoreNews_triggered()
{
	openWebPage(QUrl("http://multimc.org/posts.html"));
}

void MainWindow::newsButtonClicked()
{
	QList<NewsEntryPtr> entries = MMC->newsChecker()->getNewsEntries();
	if (entries.count() > 0)
		openWebPage(QUrl(entries[0]->link));
	else
		openWebPage(QUrl("http://multimc.org/posts.html"));
}

void MainWindow::on_actionAbout_triggered()
{
	AboutDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_mainToolBar_visibilityChanged(bool)
{
	// Don't allow hiding the main toolbar.
	// This is the only way I could find to prevent it... :/
	ui->mainToolBar->setVisible(true);
}

void MainWindow::on_actionDeleteInstance_triggered()
{
	if (m_selectedInstance)
	{
		auto response = CustomMessageBox::selectable(
			this, tr("CAREFUL"), tr("This is permanent! Are you sure?\nAbout to delete: ") +
									 m_selectedInstance->name(),
			QMessageBox::Question, QMessageBox::Yes | QMessageBox::No)->exec();
		if (response == QMessageBox::Yes)
		{
			m_selectedInstance->nuke();
		}
	}
}

void MainWindow::on_actionRenameInstance_triggered()
{
	if (m_selectedInstance)
	{
		bool ok = false;
		QString name(m_selectedInstance->name());
		name =
			QInputDialog::getText(this, tr("Instance name"), tr("Enter a new instance name."),
								  QLineEdit::Normal, name, &ok);

		if (name.length() > 0)
		{
			if (ok && name.length())
			{
				m_selectedInstance->setName(name);
				renameButton->setText(name);
			}
		}
	}
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
	if (m_selectedInstance)
	{
		QString str = m_selectedInstance->instanceRoot();
		openDirInDefaultProgram(QDir(str).absolutePath());
	}
}

void MainWindow::on_actionEditInstMods_triggered()
{
	if (m_selectedInstance)
	{
		auto dialog = m_selectedInstance->createModEditDialog(this);
		if (dialog)
			dialog->exec();
		dialog->deleteLater();
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// Save the window state and geometry.

	MMC->settings()->set("MainWindowState", saveState().toBase64());
	MMC->settings()->set("MainWindowGeometry", saveGeometry().toBase64());

	QMainWindow::closeEvent(event);
	QApplication::exit();
}
/*
void MainWindow::on_instanceView_customContextMenuRequested(const QPoint &pos)
{
	QMenu *instContextMenu = new QMenu("Instance", this);

	// Add the actions from the toolbar to the context menu.
	instContextMenu->addActions(ui->instanceToolBar->actions());

	instContextMenu->exec(view->mapToGlobal(pos));
}
*/
void MainWindow::instanceActivated(QModelIndex index)
{
	if (!index.isValid())
		return;

	BaseInstance *inst =
		(BaseInstance *)index.data(InstanceList::InstancePointerRole).value<void *>();

	NagUtils::checkJVMArgs(inst->settings().get("JvmArgs").toString(), this);

	doLaunch();
}

void MainWindow::on_actionLaunchInstance_triggered()
{
	if (m_selectedInstance)
	{
		NagUtils::checkJVMArgs(m_selectedInstance->settings().get("JvmArgs").toString(), this);
		doLaunch();
	}
}

void MainWindow::on_actionLaunchInstanceOffline_triggered()
{
	if (m_selectedInstance)
	{
		NagUtils::checkJVMArgs(m_selectedInstance->settings().get("JvmArgs").toString(), this);
		doLaunch(false);
	}
}

void MainWindow::doLaunch(bool online, BaseProfilerFactory *profiler)
{
	if (!m_selectedInstance)
		return;

	// Find an account to use.
	std::shared_ptr<MojangAccountList> accounts = MMC->accounts();
	MojangAccountPtr account = accounts->activeAccount();
	if (accounts->count() <= 0)
	{
		// Tell the user they need to log in at least one account in order to play.
		auto reply = CustomMessageBox::selectable(
			this, tr("No Accounts"),
			tr("In order to play Minecraft, you must have at least one Mojang or Minecraft "
			   "account logged in to MultiMC."
			   "Would you like to open the account manager to add an account now?"),
			QMessageBox::Information, QMessageBox::Yes | QMessageBox::No)->exec();

		if (reply == QMessageBox::Yes)
		{
			// Open the account manager.
			on_actionManageAccounts_triggered();
		}
	}
	else if (account.get() == nullptr)
	{
		// If no default account is set, ask the user which one to use.
		AccountSelectDialog selectDialog(tr("Which account would you like to use?"),
										 AccountSelectDialog::GlobalDefaultCheckbox, this);

		selectDialog.exec();

		// Launch the instance with the selected account.
		account = selectDialog.selectedAccount();

		// If the user said to use the account as default, do that.
		if (selectDialog.useAsGlobalDefault() && account.get() != nullptr)
			accounts->setActiveAccount(account->username());
	}

	// if no account is selected, we bail
	if (!account.get())
		return;

	// we try empty password first :)
	QString password;
	// we loop until the user succeeds in logging in or gives up
	bool tryagain = true;
	// the failure. the default failure.
	QString failReason = tr("Your account is currently not logged in. Please enter "
							"your password to log in again.");

	while (tryagain)
	{
		AuthSessionPtr session(new AuthSession());
		session->wants_online = online;
		auto task = account->login(session, password);
		if (task)
		{
			// We'll need to validate the access token to make sure the account
			// is still logged in.
			ProgressDialog progDialog(this);
			if (online)
				progDialog.setSkipButton(true, tr("Play Offline"));
			progDialog.exec(task.get());
			if (!task->successful())
			{
				failReason = task->failReason();
			}
		}
		switch (session->status)
		{
		case AuthSession::Undetermined:
		{
			QLOG_ERROR() << "Received undetermined session status during login. Bye.";
			tryagain = false;
			break;
		}
		case AuthSession::RequiresPassword:
		{
			EditAccountDialog passDialog(failReason, this, EditAccountDialog::PasswordField);
			if (passDialog.exec() == QDialog::Accepted)
			{
				password = passDialog.password();
			}
			else
			{
				tryagain = false;
			}
			break;
		}
		case AuthSession::PlayableOffline:
		{
			// we ask the user for a player name
			bool ok = false;
			QString usedname = session->player_name;
			QString name = QInputDialog::getText(this, tr("Player name"),
												 tr("Choose your offline mode player name."),
												 QLineEdit::Normal, session->player_name, &ok);
			if (!ok)
			{
				tryagain = false;
				break;
			}
			if (name.length())
			{
				usedname = name;
			}
			session->MakeOffline(usedname);
			// offline flavored game from here :3
		}
		case AuthSession::PlayableOnline:
		{
			// update first if the server actually responded
			if (session->auth_server_online)
			{
				updateInstance(m_selectedInstance, session, profiler);
			}
			else
			{
				launchInstance(m_selectedInstance, session, profiler);
			}
			tryagain = false;
		}
		}
	}
}

void MainWindow::updateInstance(BaseInstance *instance, AuthSessionPtr session, BaseProfilerFactory *profiler)
{
	auto updateTask = instance->doUpdate();
	if (!updateTask)
	{
		launchInstance(instance, session, profiler);
		return;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), &Task::succeeded, [this, instance, session, profiler]
	{ launchInstance(instance, session, profiler); });
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	tDialog.exec(updateTask.get());
}

void MainWindow::launchInstance(BaseInstance *instance, AuthSessionPtr session, BaseProfilerFactory *profiler)
{
	Q_ASSERT_X(instance != NULL, "launchInstance", "instance is NULL");
	Q_ASSERT_X(session.get() != nullptr, "launchInstance", "session is NULL");

	proc = instance->prepareForLaunch(session);
	if (!proc)
		return;

	this->hide();

	console = new ConsoleWindow(proc);
	connect(console, SIGNAL(isClosing()), this, SLOT(instanceEnded()));
	connect(console, &ConsoleWindow::uploadScreenshots, this, &MainWindow::on_actionScreenshots_triggered);

	proc->setLogin(session);
	proc->arm();

	if (profiler)
	{
		QString error;
		if (!profiler->check(&error))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't start profiler: %1").arg(error));
			proc->abort();
			return;
		}
		BaseProfiler *profilerInstance = profiler->createProfiler(instance, this);
		QProgressDialog dialog;
		dialog.setMinimum(0);
		dialog.setMaximum(0);
		dialog.setValue(0);
		dialog.setLabelText(tr("Waiting for profiler..."));
		connect(&dialog, &QProgressDialog::canceled, profilerInstance, &BaseProfiler::abortProfiling);
		dialog.show();
		connect(profilerInstance, &BaseProfiler::readyToLaunch, [&dialog, this](const QString &message)
		{
			dialog.accept();
			QMessageBox msg;
			msg.setText(tr("The launch of Minecraft itself is delayed until you press the "
						   "button. This is the right time to setup the profiler, as the "
						   "profiler server is running now.\n\n%1").arg(message));
			msg.setWindowTitle(tr("Waiting"));
			msg.setIcon(QMessageBox::Information);
			msg.addButton(tr("Launch"), QMessageBox::AcceptRole);
			msg.exec();
			proc->launch();
		});
		connect(profilerInstance, &BaseProfiler::abortLaunch, [&dialog, this](const QString &message)
		{
			dialog.accept();
			QMessageBox msg;
			msg.setText(tr("Couldn't start the profiler: %1").arg(message));
			msg.setWindowTitle(tr("Error"));
			msg.setIcon(QMessageBox::Critical);
			msg.addButton(QMessageBox::Ok);
			msg.exec();
			proc->abort();
		});
		profilerInstance->beginProfiling(proc);
		dialog.exec();
	}
	else
	{
		proc->launch();
	}
}

void MainWindow::onGameUpdateError(QString error)
{
	CustomMessageBox::selectable(this, tr("Error updating instance"), error,
								 QMessageBox::Warning)->show();
}

void MainWindow::taskStart()
{
	// Nothing to do here yet.
}

void MainWindow::taskEnd()
{
	QObject *sender = QObject::sender();
	if (sender == m_versionLoadTask)
		m_versionLoadTask = NULL;

	sender->deleteLater();
}

void MainWindow::startTask(Task *task)
{
	connect(task, SIGNAL(started()), SLOT(taskStart()));
	connect(task, SIGNAL(succeeded()), SLOT(taskEnd()));
	connect(task, SIGNAL(failed(QString)), SLOT(taskEnd()));
	task->start();
}

// Create A Desktop Shortcut
void MainWindow::on_actionMakeDesktopShortcut_triggered()
{
	QString name("Test");
	name = QInputDialog::getText(this, tr("MultiMC Shortcut"), tr("Enter a Shortcut Name."),
								 QLineEdit::Normal, name);

	Util::createShortCut(Util::getDesktopDir(), QApplication::instance()->applicationFilePath(),
						 QStringList() << "-dl" << QDir::currentPath() << "test", name,
						 "application-x-octet-stream");

	CustomMessageBox::selectable(
		this, tr("Not useful"),
		tr("A Dummy Shortcut was created. it will not do anything productive"),
		QMessageBox::Warning)->show();
}

// BrowserDialog
void MainWindow::openWebPage(QUrl url)
{
	QDesktopServices::openUrl(url);
}

void MainWindow::on_actionChangeInstMCVersion_triggered()
{
	if (view->selectionModel()->selectedIndexes().count() < 1)
		return;

	VersionSelectDialog vselect(m_selectedInstance->versionList().get(),
								tr("Change Minecraft version"), this);
	vselect.setFilter(1, "OneSix");
	if (!vselect.exec() || !vselect.selectedVersion())
		return;

	if (!MMC->accounts()->anyAccountIsValid())
	{
		CustomMessageBox::selectable(
			this, tr("Error"),
			tr("MultiMC cannot download Minecraft or update instances unless you have at least "
			   "one account added.\nPlease add your Mojang or Minecraft account."),
			QMessageBox::Warning)->show();
		return;
	}

	if (m_selectedInstance->versionIsCustom())
	{
		auto result = CustomMessageBox::selectable(
			this, tr("Are you sure?"),
			tr("This will remove any library/version customization you did previously. "
			   "This includes things like Forge install and similar."),
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Abort,
			QMessageBox::Abort)->exec();

		if (result != QMessageBox::Ok)
			return;
	}
	m_selectedInstance->setIntendedVersionId(vselect.selectedVersion()->descriptor());

	auto updateTask = m_selectedInstance->doUpdate();
	if (!updateTask)
	{
		return;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	tDialog.exec(updateTask.get());
}

void MainWindow::on_actionChangeInstLWJGLVersion_triggered()
{
	if (!m_selectedInstance)
		return;

	LWJGLSelectDialog lselect(this);
	lselect.exec();
	if (lselect.result() == QDialog::Accepted)
	{
		LegacyInstance *linst = (LegacyInstance *)m_selectedInstance;
		linst->setLWJGLVersion(lselect.selectedVersion());
	}
}

void MainWindow::on_actionInstanceSettings_triggered()
{
	if (view->selectionModel()->selectedIndexes().count() < 1)
		return;

	InstanceSettings settings(&m_selectedInstance->settings(), this);
	settings.setWindowTitle(tr("Instance settings"));
	settings.exec();
}

void MainWindow::instanceChanged(const QModelIndex &current, const QModelIndex &previous)
{
	if (current.isValid() &&
		nullptr != (m_selectedInstance =
						(BaseInstance *)current.data(InstanceList::InstancePointerRole)
							.value<void *>()))
	{
		ui->instanceToolBar->setEnabled(m_selectedInstance->canLaunch());
		renameButton->setText(m_selectedInstance->name());
		ui->actionChangeInstLWJGLVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstLWJGLVersion"));
		ui->actionEditInstMods->setEnabled(
			m_selectedInstance->menuActionEnabled("actionEditInstMods"));
		ui->actionChangeInstMCVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstMCVersion"));
		m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
		updateInstanceToolIcon(m_selectedInstance->iconKey());

		updateToolsMenu();

		MMC->settings()->set("SelectedInstance", m_selectedInstance->id());
	}
	else
	{
		selectionBad();

		MMC->settings()->set("SelectedInstance", QString());
	}
}

void MainWindow::selectionBad()
{
	// start by reseting everything...
	m_selectedInstance = nullptr;

	statusBar()->clearMessage();
	ui->instanceToolBar->setEnabled(false);
	renameButton->setText(tr("Rename Instance"));
	updateInstanceToolIcon("infinity");

	// ...and then see if we can enable the previously selected instance
	setSelectedInstanceById(MMC->settings()->get("SelectedInstance").toString());
}

void MainWindow::on_actionEditInstNotes_triggered()
{
	if (!m_selectedInstance)
		return;
	LegacyInstance *linst = (LegacyInstance *)m_selectedInstance;

	EditNotesDialog noteedit(linst->notes(), linst->name(), this);
	noteedit.exec();
	if (noteedit.result() == QDialog::Accepted)
	{

		linst->setNotes(noteedit.getText());
	}
}

void MainWindow::instanceEnded()
{
	this->show();
}

void MainWindow::checkMigrateLegacyAssets()
{
	int legacyAssets = AssetsUtils::findLegacyAssets();
	if (legacyAssets > 0)
	{
		ProgressDialog migrateDlg(this);
		AssetsMigrateTask migrateTask(legacyAssets, &migrateDlg);
		{
			ThreadTask threadTask(&migrateTask);

			if (migrateDlg.exec(&threadTask))
			{
				QLOG_INFO() << "Assets migration task completed successfully";
			}
			else
			{
				QLOG_INFO() << "Assets migration task reported failure";
			}
		}
	}
	else
	{
		QLOG_INFO() << "Didn't find any legacy assets to migrate";
	}
}

void MainWindow::checkSetDefaultJava()
{
	bool askForJava = false;
	{
		QString currentHostName = QHostInfo::localHostName();
		QString oldHostName = MMC->settings()->get("LastHostname").toString();
		if (currentHostName != oldHostName)
		{
			MMC->settings()->set("LastHostname", currentHostName);
			askForJava = true;
		}
	}

	{
		QString currentJavaPath = MMC->settings()->get("JavaPath").toString();
		if (currentJavaPath.isEmpty())
		{
			askForJava = true;
		}
	}

	if (askForJava)
	{
		QLOG_DEBUG() << "Java path needs resetting, showing Java selection dialog...";

		JavaVersionPtr java;

		VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this,
									false);
		vselect.setResizeOn(2);
		vselect.exec();

		if (vselect.selectedVersion())
			java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		else
		{
			CustomMessageBox::selectable(
				this, tr("Invalid version selected"),
				tr("You didn't select a valid Java version, so MultiMC will "
				   "select the default. "
				   "You can change this in the settings dialog."),
				QMessageBox::Warning)->show();

			JavaUtils ju;
			java = ju.GetDefaultJava();
		}
		if (java)
			MMC->settings()->set("JavaPath", java->path);
		else
			MMC->settings()->set("JavaPath", QString("java"));
	}
}

void MainWindow::on_actionScreenshots_triggered()
{
	if (!m_selectedInstance)
		return;
	ScreenshotList *list = new ScreenshotList(m_selectedInstance);
	Task *task = list->load();
	ProgressDialog prog(this);
	prog.exec(task);
	if (!task->successful())
	{
		CustomMessageBox::selectable(this, tr("Failed to load screenshots!"),
									 task->failReason(), QMessageBox::Warning)->exec();
		return;
	}
	ScreenshotDialog dialog(list, this);
	if (dialog.exec() == ScreenshotDialog::Accepted)
	{
		CustomMessageBox::selectable(this, tr("Done uploading!"), dialog.message(),
									 QMessageBox::Information)->exec();
	}
}
