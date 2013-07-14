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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

#include "osutils.h"
#include "userutils.h"
#include "pathutils.h"

#include "gui/settingsdialog.h"
#include "gui/newinstancedialog.h"
#include "gui/logindialog.h"
#include "gui/taskdialog.h"
#include "gui/browserdialog.h"
#include "gui/aboutdialog.h"
#include "gui/versionselectdialog.h"
#include "gui/lwjglselectdialog.h"
#include "gui/consolewindow.h"
#include "gui/modeditwindow.h"
#include "gui/instancesettings.h"

#include "kcategorizedview.h"
#include "kcategorydrawer.h"

#include "instancelist.h"
#include "appsettings.h"
#include "version.h"

#include "logintask.h"
#include "gameupdatetask.h"

#include "instance.h"
#include "minecraftprocess.h"

#include "instancemodel.h"
#include "instancedelegate.h"

#include "minecraftversionlist.h"
#include "lwjglversionlist.h"

// Opens the given file in the default application.
// TODO: Move this somewhere.
void openFileInDefaultProgram ( QString filename );
void openDirInDefaultProgram ( QString dirpath, bool ensureExists = false );

MainWindow::MainWindow ( QWidget *parent ) :
	QMainWindow ( parent ),
	ui ( new Ui::MainWindow ),
	instList ( globalSettings->get ( "InstanceDir" ).toString() )
{
	ui->setupUi ( this );
	
	// Set active instance to null.
	m_activeInst = NULL;
	
	// Create the widget
	view = new KCategorizedView ( ui->centralWidget );
	drawer = new KCategoryDrawer ( view );
	/*
	QPalette pal = view->palette();
	pal.setBrush(QPalette::Base, QBrush(QPixmap(QString::fromUtf8(":/backgrounds/kitteh"))));
	view->setPalette(pal);
	*/
	
//	view->setStyleSheet(
//		"QListView\
//		{\
//			background-image: url(:/backgrounds/kitteh);\
//			background-attachment: fixed;\
//			background-clip: padding;\
//			background-position: top right;\
//			background-repeat: none;\
//			background-color:palette(base);\
//		}");
	
	view->setSelectionMode ( QAbstractItemView::SingleSelection );
	//view->setSpacing( KDialog::spacingHint() );
	view->setCategoryDrawer ( drawer );
	view->setCollapsibleBlocks ( true );
	view->setViewMode ( QListView::IconMode );
	view->setFlow ( QListView::LeftToRight );
	view->setWordWrap(true);
	view->setMouseTracking ( true );
	view->viewport()->setAttribute ( Qt::WA_Hover );
	auto delegate = new ListViewDelegate();
	view->setItemDelegate(delegate);
	view->setSpacing(10);
	view->setUniformItemWidths(true);

	model = new InstanceModel ( instList,this );
	proxymodel = new InstanceProxyModel ( this );
	proxymodel->setSortRole ( KCategorizedSortFilterProxyModel::CategorySortRole );
	proxymodel->setFilterRole ( KCategorizedSortFilterProxyModel::CategorySortRole );
	//proxymodel->setDynamicSortFilter ( true );
	proxymodel->setSourceModel ( model );
	proxymodel->sort ( 0 );

	view->setFrameShape ( QFrame::NoFrame );

	ui->horizontalLayout->addWidget ( view );
	setWindowTitle ( QString ( "MultiMC %1" ).arg ( Version::current.toString() ) );
	// TODO: Make this work with the new settings system.
//	restoreGeometry(settings->getConfig().value("MainWindowGeometry", saveGeometry()).toByteArray());
//	restoreState(settings->getConfig().value("MainWindowState", saveState()).toByteArray());
	view->setModel ( proxymodel );
	connect(view, SIGNAL(doubleClicked(const QModelIndex &)),
        this, SLOT(instanceActivated(const QModelIndex &)));

    connect(view, SIGNAL(clicked(const QModelIndex &)),
        this, SLOT(instanceChanged(const QModelIndex &)));
	
	// Load the instances.
	instList.loadList();
	// just a test
	/*
	instList.at(0)->setGroup("TEST GROUP");
	instList.at(0)->setName("TEST ITEM");
	*/
	
	if (!MinecraftVersionList::getMainList().isLoaded())
	{
		m_versionLoadTask = MinecraftVersionList::getMainList().getLoadTask();
		startTask(m_versionLoadTask);
	}
	
	if (!LWJGLVersionList::get().isLoaded())
	{
		LWJGLVersionList::get().loadList();
	}
}

MainWindow::~MainWindow()
{
	delete ui;
	delete proxymodel;
	delete model;
	delete drawer;
}

void MainWindow::instanceActivated ( QModelIndex index )
{
	if(!index.isValid())
		return;
	Instance * inst = (Instance *) index.data(InstanceModel::InstancePointerRole).value<void *>();
	doLogin();
}

void MainWindow::on_actionAddInstance_triggered()
{
	if (!MinecraftVersionList::getMainList().isLoaded() &&
		m_versionLoadTask && m_versionLoadTask->isRunning())
	{
		QEventLoop waitLoop;
		waitLoop.connect(m_versionLoadTask, SIGNAL(ended()), SLOT(quit()));
		waitLoop.exec();
	}
	
	NewInstanceDialog *newInstDlg = new NewInstanceDialog ( this );
	if (newInstDlg->exec())
	{
		Instance *newInstance = NULL;
		
		QString instDirName = DirNameFromString(newInstDlg->instName());
		QString instDir = PathCombine(globalSettings->get("InstanceDir").toString(), 
									  instDirName);
		
		InstanceLoader::InstLoaderError error = InstanceLoader::get().
				createInstance(newInstance, instDir);
		
		if (error == InstanceLoader::NoError)
		{
			newInstance->setName(newInstDlg->instName());
			newInstance->setIntendedVersion(newInstDlg->selectedVersion()->descriptor());
			instList.add(InstancePtr(newInstance));
		}
		else
		{
			QString errorMsg = QString("Failed to create instance %1: ").
					arg(instDirName);
			
			switch (error)
			{
			case InstanceLoader::InstExists:
				errorMsg += "An instance with the given directory name already exists.";
				break;
				
			case InstanceLoader::CantCreateDir:
				errorMsg += "Failed to create the instance directory.";
				break;
				
			default:
				errorMsg += QString("Unknown instance loader error %1").
						arg(error);
				break;
			}
			
			QMessageBox::warning(this, "Error", errorMsg);
		}
	}
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
	Instance* inst = selectedInstance();
	if(inst)
	{
		bool ok = false;
		QString name ( inst->group() );
		QInputDialog dlg(this);
		dlg.result();
		name = QInputDialog::getText ( this, tr ( "Group name" ), tr ( "Enter a new group name." ),
									   QLineEdit::Normal, name, &ok );
		if(ok)
			inst->setGroup(name);
	}
}


void MainWindow::on_actionViewInstanceFolder_triggered()
{
	QString str = globalSettings->get ( "InstanceDir" ).toString();
	openDirInDefaultProgram ( str );
}

void MainWindow::on_actionRefresh_triggered()
{
	instList.loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
	openDirInDefaultProgram ( globalSettings->get ( "CentralModsDir" ).toString() , true);
}

void MainWindow::on_actionCheckUpdate_triggered()
{

}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog dialog ( this );
	dialog.exec();
}

void MainWindow::on_actionReportBug_triggered()
{
	//QDesktopServices::openUrl(QUrl("http://bugs.forkk.net/"));
	openWebPage ( QUrl ( "http://bugs.forkk.net/" ) );
}

void MainWindow::on_actionNews_triggered()
{
	//QDesktopServices::openUrl(QUrl("http://news.forkk.net/"));
	openWebPage ( QUrl ( "http://news.forkk.net/" ) );
}

void MainWindow::on_actionAbout_triggered()
{
	AboutDialog dialog ( this );
	dialog.exec();
}

void MainWindow::on_mainToolBar_visibilityChanged ( bool )
{
	// Don't allow hiding the main toolbar.
	// This is the only way I could find to prevent it... :/
	ui->mainToolBar->setVisible ( true );
}

void MainWindow::on_actionDeleteInstance_triggered()
{
	Instance* inst = selectedInstance();
	if (inst)
	{
		int response = QMessageBox::question(this, "CAREFUL", 
						     QString("This is permanent! Are you sure?\nAbout to delete: ") + inst->name());
		if (response == QMessageBox::Yes)
		{
			QDir(inst->rootDir()).removeRecursively();
			instList.loadList();
		}
	}
}

void MainWindow::on_actionRenameInstance_triggered()
{
	Instance* inst = selectedInstance();
	if(inst)
	{
		bool ok = false;
		QString name ( inst->name() );
		name = QInputDialog::getText ( this, tr ( "Instance name" ), tr ( "Enter a new instance name." ),
									   QLineEdit::Normal, name, &ok );
		
		if (name.length() > 0)
		{
			if(ok && name.length() && name.length() <= 25)
				inst->setName(name);
		}
	}
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
	Instance* inst = selectedInstance();
	if(inst)
	{
		QString str = inst->rootDir();
		openDirInDefaultProgram ( QDir(str).absolutePath() );
	}
}

void MainWindow::on_actionEditInstMods_triggered()
{
	Instance* inst = selectedInstance();
	if (inst)
	{
		ModEditWindow dialog ( this, inst );
		dialog.exec();
	}
}

void MainWindow::closeEvent ( QCloseEvent *event )
{
	// Save the window state and geometry.
	// TODO: Make this work with the new settings system.
//	settings->getConfig().setValue("MainWindowGeometry", saveGeometry());
//	settings->getConfig().setValue("MainWindowState", saveState());
	QMainWindow::closeEvent ( event );
}

void MainWindow::on_instanceView_customContextMenuRequested ( const QPoint &pos )
{
	QMenu *instContextMenu = new QMenu ( "Instance", this );

	// Add the actions from the toolbar to the context menu.
	instContextMenu->addActions ( ui->instanceToolBar->actions() );

	instContextMenu->exec ( view->mapToGlobal ( pos ) );
}

Instance* MainWindow::selectedInstance()
{
	QAbstractItemView * iv = view;
	auto smodel = iv->selectionModel();
	QModelIndex mindex;
	if(smodel->hasSelection())
	{
		auto rows = smodel->selectedRows();
		mindex = rows.at(0);
	}
	
	if(mindex.isValid())
	{
		return (Instance *) mindex.data(InstanceModel::InstancePointerRole).value<void *>();
	}
	else
		return nullptr;
}


void MainWindow::on_actionLaunchInstance_triggered()
{
	Instance* inst = selectedInstance();
	if(inst)
	{
		doLogin();
	}
}

void MainWindow::doLogin(const QString& errorMsg)
{
	if (!selectedInstance())
		return;
	
	LoginDialog* loginDlg = new LoginDialog(this, errorMsg);
	if (loginDlg->exec())
	{
		UserInfo uInfo(loginDlg->getUsername(), loginDlg->getPassword());

		TaskDialog* tDialog = new TaskDialog(this);
		LoginTask* loginTask = new LoginTask(uInfo, tDialog);
		connect(loginTask, SIGNAL(loginComplete(LoginResponse)),
				SLOT(onLoginComplete(LoginResponse)), Qt::QueuedConnection);
		connect(loginTask, SIGNAL(loginFailed(QString)),
				SLOT(doLogin(QString)), Qt::QueuedConnection);
		m_activeInst = selectedInstance();
		tDialog->exec(loginTask);
	}
}

void MainWindow::onLoginComplete(LoginResponse response)
{
	Q_ASSERT_X(m_activeInst != NULL, "onLoginComplete", "no active instance is set");
	
	if (!m_activeInst->shouldUpdate())
	{
		launchInstance(m_activeInst, response);
	}
	else
	{
		TaskDialog *tDialog = new TaskDialog(this);
		GameUpdateTask *updateTask = new GameUpdateTask(response, m_activeInst);
		connect(updateTask, SIGNAL(gameUpdateComplete(LoginResponse)), 
				SLOT(onGameUpdateComplete(LoginResponse)));
		connect(updateTask, SIGNAL(gameUpdateError(QString)), SLOT(onGameUpdateError(QString)));
		tDialog->exec(updateTask);
	}
}

void MainWindow::onGameUpdateComplete(LoginResponse response)
{
	launchInstance(response);
}

void MainWindow::onGameUpdateError(QString error)
{
	QMessageBox::warning(this, "Error downloading instance", error);
}


void MainWindow::launchInstance(LoginResponse response)
{
	Q_ASSERT_X(m_activeInst != NULL, "onLoginComplete", "no active instance is set");
	launchInstance(m_activeInst, response);
}

void MainWindow::launchInstance(QString instID, LoginResponse response)
{
	Instance *instance = instList.getInstanceById(instID).data();
	Q_ASSERT_X(instance != NULL, "launchInstance", "instance ID does not correspond to a valid instance");
	launchInstance(instance, response);
}

void MainWindow::launchInstance(Instance *instance, LoginResponse response)
{
	Q_ASSERT_X(instance != NULL, "launchInstance", "instance is NULL");
	
	console = new ConsoleWindow();
	proc = new MinecraftProcess(instance, response.username(), response.sessionID());
	
	console->show();
	//connect(proc, SIGNAL(ended()), SLOT(onTerminated()));
	connect(proc, SIGNAL(log(QString, MessageLevel::Enum)), 
			console, SLOT(write(QString, MessageLevel::Enum)));
	proc->launch();
}

void MainWindow::taskStart(Task *task)
{
	// Nothing to do here yet.
}

void MainWindow::taskEnd(Task *task)
{
	if (task == m_versionLoadTask)
		m_versionLoadTask = NULL;
	
	delete task;
}

void MainWindow::startTask(Task *task)
{
	connect(task, SIGNAL(started(Task*)), SLOT(taskStart(Task*)));
	connect(task, SIGNAL(ended(Task*)), SLOT(taskEnd(Task*)));
	task->startTask();
}


// Create A Desktop Shortcut
void MainWindow::on_actionMakeDesktopShortcut_triggered()
{
	QString name ( "Test" );
	name = QInputDialog::getText ( this, tr ( "MultiMC Shortcut" ), tr ( "Enter a Shortcut Name." ), QLineEdit::Normal, name );

	Util::createShortCut ( Util::getDesktopDir(), QApplication::instance()->applicationFilePath(), QStringList() << "-dl" << QDir::currentPath() << "test", name, "application-x-octet-stream" );

	QMessageBox::warning ( this, "Not useful", "A Dummy Shortcut was created. it will not do anything productive" );
}

// BrowserDialog
void MainWindow::openWebPage ( QUrl url )
{
	BrowserDialog *browser = new BrowserDialog ( this );

	browser->load ( url );
	browser->exec();
}

void openDirInDefaultProgram ( QString path, bool ensureExists )
{
	QDir parentPath;
	QDir dir( path );
	if(!dir.exists())
	{
		parentPath.mkpath(dir.absolutePath());
	}
	QDesktopServices::openUrl ( "file:///" + dir.absolutePath() );
}

void openFileInDefaultProgram ( QString filename )
{
	QDesktopServices::openUrl ( "file:///" + QFileInfo ( filename ).absolutePath() );
}

void MainWindow::on_actionChangeInstMCVersion_triggered()
{
	if (view->selectionModel()->selectedIndexes().count() < 1)
		return;
	
	Instance *inst = selectedInstance();
	
	VersionSelectDialog *vselect = new VersionSelectDialog(inst->versionList(), this);
	if (vselect->exec() && vselect->selectedVersion())
	{
		inst->setIntendedVersion(vselect->selectedVersion()->descriptor());
	}
}

void MainWindow::on_actionChangeInstLWJGLVersion_triggered()
{
	Instance *inst = selectedInstance();
	
	if (!inst)
		return;
	
	LWJGLSelectDialog *lselect = new LWJGLSelectDialog(this);
	if (lselect->exec())
	{
		
	}
}

void MainWindow::on_actionInstanceSettings_triggered()
{
    if (view->selectionModel()->selectedIndexes().count() < 1)
        return;

    Instance *inst = selectedInstance();
    SettingsObject *s;
    s = &inst->settings();
    InstanceSettings *settings = new InstanceSettings (this);
    settings->setWindowTitle(QString("Instance settings"));
    settings->loadSettings(s);
    if (settings->exec()) {
        settings->applySettings(s);
    }
    delete settings;
}

void MainWindow::instanceChanged(QModelIndex idx) {
    ui->instanceToolBar->setEnabled(idx.isValid());
}
