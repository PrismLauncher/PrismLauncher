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
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QToolButton>
#include <QWidgetAction>

#include "osutils.h"
#include "userutils.h"
#include "pathutils.h"

#include "categorizedview.h"
#include "categorydrawer.h"

#include "gui/Platform.h"

#include "gui/widgets/InstanceDelegate.h"
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

#include "gui/ConsoleWindow.h"

#include "logic/lists/InstanceList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/icons/IconList.h"
#include "logic/lists/JavaVersionList.h"

#include "logic/auth/flows/AuthenticateTask.h"
#include "logic/auth/flows/RefreshTask.h"
#include "logic/auth/flows/ValidateTask.h"

#include "logic/updater/DownloadUpdateTask.h"

#include "logic/net/URLConstants.h"

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
#include <logic/tasks/ThreadTask.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	setWindowTitle(QString("MultiMC %1").arg(MMC->version().toString()));

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

	// Create the instance list widget
	{
		view = new KCategorizedView(ui->centralWidget);
		drawer = new KCategoryDrawer(view);

		view->setSelectionMode(QAbstractItemView::SingleSelection);
		view->setCategoryDrawer(drawer);
		view->setCollapsibleBlocks(true);
		view->setViewMode(QListView::IconMode);
		view->setFlow(QListView::LeftToRight);
		view->setWordWrap(true);
		view->setMouseTracking(true);
		view->viewport()->setAttribute(Qt::WA_Hover);
		auto delegate = new ListViewDelegate();
		view->setItemDelegate(delegate);
		view->setSpacing(10);
		view->setUniformItemWidths(true);

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

	m_statusLeft = new QLabel(tr("Instance type"), this);
	m_statusRight = new QLabel(this);
	m_statusRight->setAlignment(Qt::AlignRight);
	statusBar()->addPermanentWidget(m_statusLeft, 1);
	statusBar()->addPermanentWidget(m_statusRight, 0);

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
	accountMenuButton->setIcon(
		QPixmap(":/icons/toolbar/noaccount").scaled(48, 48, Qt::KeepAspectRatio));

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

	std::shared_ptr<MojangAccountList> accounts = MMC->accounts();

	// TODO: Nicer way to iterate?
	for (int i = 0; i < accounts->count(); i++)
	{
		MojangAccountPtr account = accounts->at(i);
		if (account != nullptr)
		{
			auto job = new NetJob("Startup player skins: " + account->username());

			for (AccountProfile profile : account->profiles())
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
	}

	const QString currentInstanceId = MMC->settings()->get("SelectedInstance").toString();
	if (!currentInstanceId.isNull())
	{
		const QModelIndex index = MMC->instances()->getInstanceIndexById(currentInstanceId);
		if (index.isValid())
		{
			const QModelIndex mappedIndex = proxymodel->mapFromSource(index);
			view->setCurrentIndex(mappedIndex);
		}
		else
		{
			view->setCurrentIndex(proxymodel->index(0, 0));
		}
	}
	else
	{
		view->setCurrentIndex(proxymodel->index(0, 0));
	}

	// removing this looks stupid
	view->setFocus();
}

MainWindow::~MainWindow()
{
	delete ui;
	delete proxymodel;
	delete drawer;
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
	action->setIcon(QPixmap(":/icons/toolbar/noaccount").scaled(48, 48, Qt::KeepAspectRatio));
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
	accountMenuButton->setIcon(
		QPixmap(":/icons/toolbar/noaccount").scaled(48, 48, Qt::KeepAspectRatio));
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
		if (installOnExit)
			MMC->setUpdateOnExit(updateTask.updateFilesDir());
		else
			MMC->installUpdates(updateTask.updateFilesDir(), true);
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
		view->setStyleSheet("QListView"
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
		auto update = newInstance->doUpdate(false);
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
		auto ico = MMC->icons()->getIcon(dlg.selectedIconKey);
		ui->actionChangeInstIcon->setIcon(ico);
	}
}

void MainWindow::iconUpdated(QString icon)
{
	if(icon == m_currentInstIcon)
	{
		ui->actionChangeInstIcon->setIcon(MMC->icons()->getIcon(m_currentInstIcon));
	}
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
	m_currentInstIcon = new_icon;
	ui->actionChangeInstIcon->setIcon(MMC->icons()->getIcon(m_currentInstIcon));
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
}

void MainWindow::on_actionManageAccounts_triggered()
{
	AccountListDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_actionReportBug_triggered()
{
	openWebPage(QUrl("http://multimc.myjetbrains.com/youtrack/dashboard#newissue=yes"));
}

void MainWindow::on_actionNews_triggered()
{
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

void MainWindow::doLaunch()
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

	QString failReason = tr("Your account is currently not logged in. Please enter "
							"your password to log in again.");
	// do the login. if the account has an access token, try to refresh it first.
	if (account->accountStatus() != NotVerified)
	{
		// We'll need to validate the access token to make sure the account is still logged in.
		ProgressDialog progDialog(this);
		progDialog.setSkipButton(true, tr("Play Offline"));
		auto task = account->login();
		progDialog.exec(task.get());

		auto status = account->accountStatus();
		if (status != NotVerified)
		{
			updateInstance(m_selectedInstance, account);
		}
		else
		{
			if (!task->successful())
			{
				failReason = task->failReason();
			}
			if (loginWithPassword(account, failReason))
				updateInstance(m_selectedInstance, account);
		}
		// in any case, revert from online to verified.
		account->downgrade();
	}
	else
	{
		if (loginWithPassword(account, failReason))
		{
			updateInstance(m_selectedInstance, account);
			account->downgrade();
		}
		// in any case, revert from online to verified.
		account->downgrade();
	}
}

bool MainWindow::loginWithPassword(MojangAccountPtr account, const QString &errorMsg)
{
	EditAccountDialog passDialog(errorMsg, this, EditAccountDialog::PasswordField);
	if (passDialog.exec() == QDialog::Accepted)
	{
		// To refresh the token, we just create an authenticate task with the given account and
		// the user's password.
		ProgressDialog progDialog(this);
		auto task = account->login(passDialog.password());
		progDialog.exec(task.get());
		if (task->successful())
			return true;
		else
		{
			// If the authentication task failed, recurse with the task's error message.
			return loginWithPassword(account, task->failReason());
		}
	}
	return false;
}

void MainWindow::updateInstance(BaseInstance *instance, MojangAccountPtr account)
{
	bool only_prepare = account->accountStatus() != Online;
	auto updateTask = instance->doUpdate(only_prepare);
	if (!updateTask)
	{
		launchInstance(instance, account);
		return;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), &Task::succeeded, [this, instance, account]
	{ launchInstance(instance, account); });
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	tDialog.exec(updateTask.get());
}

void MainWindow::launchInstance(BaseInstance *instance, MojangAccountPtr account)
{
	Q_ASSERT_X(instance != NULL, "launchInstance", "instance is NULL");
	Q_ASSERT_X(account.get() != nullptr, "launchInstance", "account is NULL");

	proc = instance->prepareForLaunch(account);
	if (!proc)
		return;

	this->hide();

	console = new ConsoleWindow(proc);
	connect(console, SIGNAL(isClosing()), this, SLOT(instanceEnded()));

	proc->setLogin(account);
	proc->launch();
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
	if(!vselect.exec() || !vselect.selectedVersion())
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

	auto updateTask = m_selectedInstance->doUpdate(false);
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
		ui->instanceToolBar->setEnabled(true);
		renameButton->setText(m_selectedInstance->name());
		ui->actionChangeInstLWJGLVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstLWJGLVersion"));
		ui->actionEditInstMods->setEnabled(
			m_selectedInstance->menuActionEnabled("actionEditInstMods"));
		ui->actionChangeInstMCVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstMCVersion"));
		m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
		updateInstanceToolIcon(m_selectedInstance->iconKey());

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
	m_selectedInstance = nullptr;

	statusBar()->clearMessage();
	ui->instanceToolBar->setEnabled(false);
	renameButton->setText(tr("Rename Instance"));
	updateInstanceToolIcon("infinity");
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
	if(legacyAssets > 0)
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

void MainWindow::assetsIndexStarted()
{
	m_statusRight->setText(tr("Checking assets..."));
}

void MainWindow::assetsFilesStarted()
{
	m_statusRight->setText(tr("Downloading assets..."));
}

void MainWindow::assetsFilesProgress(int succeeded, int failed, int total)
{
	QString status = tr("Downloading assets: %1 / %2").arg(succeeded + failed).arg(total);
	if (failed > 0)
		status += tr(" (%1 failed)").arg(failed);
	status += tr("...");
	m_statusRight->setText(status);
}

void MainWindow::assetsFailed()
{
	m_statusRight->setText(tr("Failed to update assets."));
}

void MainWindow::assetsFinished()
{
	m_statusRight->setText(tr("Assets up to date."));
}
