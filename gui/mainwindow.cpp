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
#include <MultiMC.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>

#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QToolButton>

#include "osutils.h"
#include "userutils.h"
#include "pathutils.h"

#include "categorizedview.h"
#include "categorydrawer.h"

#include "gui/settingsdialog.h"
#include "gui/newinstancedialog.h"
#include "gui/logindialog.h"
#include "gui/ProgressDialog.h"
#include "gui/aboutdialog.h"
#include "gui/versionselectdialog.h"
#include "gui/lwjglselectdialog.h"
#include "gui/consolewindow.h"
#include "gui/instancesettings.h"

#include "logic/lists/InstanceList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/lists/LwjglVersionList.h"
#include "logic/lists/IconList.h"

#include "logic/net/LoginTask.h"
#include "logic/BaseInstance.h"
#include "logic/InstanceFactory.h"
#include "logic/MinecraftProcess.h"
#include "logic/OneSixAssets.h"
#include "logic/OneSixUpdate.h"

#include "logic/LegacyInstance.h"

#include "instancedelegate.h"
#include "IconPickerDialog.h"
#include "LabeledToolButton.h"
#include "EditNotesDialog.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setWindowTitle(QString("MultiMC %1").arg(MMC->version().toString()));

	// Set the selected instance to null
	m_selectedInstance = nullptr;
	// Set active instance to null.
	m_activeInst = nullptr;

	// OSX magic.
	setUnifiedTitleAndToolBarOnMac(true);

	// The instance action toolbar customizations
	{
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
		proxymodel->setSortRole(KCategorizedSortFilterProxyModel::CategorySortRole);
		proxymodel->setFilterRole(KCategorizedSortFilterProxyModel::CategorySortRole);
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
	// model reset -> selection is invalid. All the instance pointers are wrong.
	// FIXME: stop using POINTERS everywhere
	connect(MMC->instances().get(), SIGNAL(dataIsInvalid()), SLOT(selectionBad()));

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
		assets_downloader = new OneSixAssets();
		assets_downloader->start();
	}
}

MainWindow::~MainWindow()
{
	delete ui;
	delete proxymodel;
	delete drawer;
	delete assets_downloader;
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

void MainWindow::instanceActivated(QModelIndex index)
{
	if (!index.isValid())
		return;
	BaseInstance *inst =
		(BaseInstance *)index.data(InstanceList::InstancePointerRole).value<void *>();
	doLogin();
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

	QString instDirName = DirNameFromString(newInstDlg.instName());
	QString instDir = PathCombine(MMC->settings()->get("InstanceDir").toString(), instDirName);

	auto &loader = InstanceFactory::get();

	auto error = loader.createInstance(newInstance, newInstDlg.selectedVersion(), instDir);
	QString errorMsg = QString("Failed to create instance %1: ").arg(instDirName);
	switch (error)
	{
	case InstanceFactory::NoCreateError:
		newInstance->setName(newInstDlg.instName());
		newInstance->setIconKey(newInstDlg.iconKey());
		MMC->instances()->add(InstancePtr(newInstance));
		return;

	case InstanceFactory::InstExists:
		errorMsg += "An instance with the given directory name already exists.";
		QMessageBox::warning(this, "Error", errorMsg);
		break;

	case InstanceFactory::CantCreateDir:
		errorMsg += "Failed to create the instance directory.";
		QMessageBox::warning(this, "Error", errorMsg);
		break;

	default:
		errorMsg += QString("Unknown instance loader error %1").arg(error);
		QMessageBox::warning(this, "Error", errorMsg);
		break;
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

void MainWindow::on_actionChangeInstGroup_triggered()
{
	if (!m_selectedInstance)
		return;

	bool ok = false;
	QString name(m_selectedInstance->group());
	name = QInputDialog::getText(this, tr("Group name"), tr("Enter a new group name."),
								 QLineEdit::Normal, name, &ok);
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
}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog dialog(this);
	dialog.exec();
}

void MainWindow::on_actionReportBug_triggered()
{
	openWebPage(QUrl("http://multimc.myjetbrains.com/youtrack/dashboard#newissue=yes"));
}

void MainWindow::on_actionNews_triggered()
{
	openWebPage(QUrl("http://forkk.net/tag/multimc.html"));
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
		int response = QMessageBox::question(
			this, "CAREFUL", QString("This is permanent! Are you sure?\nAbout to delete: ") +
								 m_selectedInstance->name());
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
	// TODO: Make this work with the new settings system.
	//	settings->getConfig().setValue("MainWindowGeometry", saveGeometry());
	//	settings->getConfig().setValue("MainWindowState", saveState());
	QMainWindow::closeEvent(event);
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
void MainWindow::on_actionLaunchInstance_triggered()
{
	if (m_selectedInstance)
	{
		doLogin();
	}
}

void MainWindow::doLogin(const QString &errorMsg)
{
	if (!m_selectedInstance)
		return;

	LoginDialog *loginDlg = new LoginDialog(this, errorMsg);
	if (!m_selectedInstance->lastLaunch())
		loginDlg->forceOnline();

	loginDlg->exec();
	if (loginDlg->result() == QDialog::Accepted)
	{
		if (loginDlg->isOnline())
		{
			UserInfo uInfo{loginDlg->getUsername(), loginDlg->getPassword()};

			ProgressDialog *tDialog = new ProgressDialog(this);
			LoginTask *loginTask = new LoginTask(uInfo, tDialog);
			connect(loginTask, SIGNAL(succeeded()), SLOT(onLoginComplete()),
					Qt::QueuedConnection);
			connect(loginTask, SIGNAL(failed(QString)), SLOT(doLogin(QString)),
					Qt::QueuedConnection);
			m_activeInst = m_selectedInstance;
			tDialog->exec(loginTask);
		}
		else
		{
			QString user = loginDlg->getUsername();
			if (user.length() == 0)
				user = QString("Offline");
			m_activeLogin = {user, QString("Offline"), QString("Player"), QString()};
			m_activeInst = m_selectedInstance;
			launchInstance(m_activeInst, m_activeLogin);
		}
	}
}

void MainWindow::onLoginComplete()
{
	if (!m_activeInst)
		return;
	LoginTask *task = (LoginTask *)QObject::sender();
	m_activeLogin = task->getResult();

	BaseUpdate *updateTask = m_activeInst->doUpdate();
	if (!updateTask)
	{
		launchInstance(m_activeInst, m_activeLogin);
	}
	else
	{
		ProgressDialog *tDialog = new ProgressDialog(this);
		connect(updateTask, SIGNAL(succeeded()), SLOT(onGameUpdateComplete()));
		connect(updateTask, SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
		tDialog->exec(updateTask);
	}
}

void MainWindow::onGameUpdateComplete()
{
	launchInstance(m_activeInst, m_activeLogin);
}

void MainWindow::onGameUpdateError(QString error)
{
	QMessageBox::warning(this, "Error updating instance", error);
}

void MainWindow::launchInstance(BaseInstance *instance, LoginResponse response)
{
	Q_ASSERT_X(instance != NULL, "launchInstance", "instance is NULL");

	proc = instance->prepareForLaunch(response);
	if (!proc)
		return;

	// Prepare GUI: If it shall stay open disable the required parts
	if (MMC->settings()->get("NoHide").toBool())
	{
		ui->actionLaunchInstance->setEnabled(false);
	}
	else
	{
		this->hide();
	}

	console = new ConsoleWindow(proc);
	console->show();
	connect(proc, SIGNAL(log(QString, MessageLevel::Enum)), console,
			SLOT(write(QString, MessageLevel::Enum)));
	connect(proc, SIGNAL(ended()), this, SLOT(instanceEnded()));
	proc->setLogin(response.username, response.session_id);
	proc->launch();
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

	QMessageBox::warning(
		this, tr("Not useful"),
		tr("A Dummy Shortcut was created. it will not do anything productive"));
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
	if (vselect.exec() && vselect.selectedVersion())
	{
		m_selectedInstance->setIntendedVersionId(vselect.selectedVersion()->descriptor());
	}
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
	settings.setWindowTitle(QString("Instance settings"));
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
		QString iconKey = m_selectedInstance->iconKey();
		renameButton->setText(m_selectedInstance->name());
		ui->actionChangeInstLWJGLVersion->setEnabled(
			m_selectedInstance->menuActionEnabled("actionChangeInstLWJGLVersion"));
		ui->actionEditInstMods->setEnabled(
			m_selectedInstance->menuActionEnabled("actionEditInstMods"));
		statusBar()->clearMessage();
		statusBar()->showMessage(m_selectedInstance->getStatusbarDescription());
		auto ico = MMC->icons()->getIcon(iconKey);
		ui->actionChangeInstIcon->setIcon(ico);
	}
	else
	{
		selectionBad();
	}
}

void MainWindow::selectionBad()
{
	m_selectedInstance = nullptr;
	QString iconKey = "infinity";
	statusBar()->clearMessage();
	ui->instanceToolBar->setEnabled(false);
	renameButton->setText(tr("Rename Instance"));
	auto ico = MMC->icons()->getIcon(iconKey);
	ui->actionChangeInstIcon->setIcon(ico);
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
	ui->actionLaunchInstance->setEnabled(m_selectedInstance);
}
