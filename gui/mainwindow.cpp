/* Copyright 2013 MultiMC Contributors
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

#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

#include "osutils.h"

#include "gui/settingsdialog.h"
#include "gui/newinstancedialog.h"
#include "gui/logindialog.h"
#include "gui/taskdialog.h"

#include "instancelist.h"
#include "data/appsettings.h"
#include "data/version.h"

#include "tasks/logintask.h"

// Opens the given file in the default application.
// TODO: Move this somewhere.
void openInDefaultProgram(QString filename);

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	instList(settings->get("InstanceDir").toString())
{
	ui->setupUi(this);
	
	setWindowTitle(QString("MultiMC %1").arg(Version::current.toString()));
	// TODO: Make this work with the new settings system.
//	restoreGeometry(settings->getConfig().value("MainWindowGeometry", saveGeometry()).toByteArray());
//	restoreState(settings->getConfig().value("MainWindowState", saveState()).toByteArray());
	
	instList.loadList();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_actionAddInstance_triggered()
{
	NewInstanceDialog *newInstDlg = new NewInstanceDialog(this);
	newInstDlg->exec();
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
	openInDefaultProgram(settings->get("InstanceDir").toString());
}

void MainWindow::on_actionRefresh_triggered()
{
	instList.loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
	openInDefaultProgram(settings->get("CentralModsDir").toString());
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
	QDesktopServices::openUrl(QUrl("http://bugs.forkk.net/"));
}

void MainWindow::on_actionNews_triggered()
{
	QDesktopServices::openUrl(QUrl("http://news.forkk.net/"));
}

void MainWindow::on_actionAbout_triggered()
{
	
}

void MainWindow::on_mainToolBar_visibilityChanged(bool)
{
	// Don't allow hiding the main toolbar.
	// This is the only way I could find to prevent it... :/
	ui->mainToolBar->setVisible(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// Save the window state and geometry.
	// TODO: Make this work with the new settings system.
//	settings->getConfig().setValue("MainWindowGeometry", saveGeometry());
//	settings->getConfig().setValue("MainWindowState", saveState());
	QMainWindow::closeEvent(event);
}

void MainWindow::on_instanceView_customContextMenuRequested(const QPoint &pos)
{
	QMenu *instContextMenu = new QMenu("Instance", this);
	
	// Add the actions from the toolbar to the context menu.
	instContextMenu->addActions(ui->instanceToolBar->actions());
	
	instContextMenu->exec(ui->instanceView->mapToGlobal(pos));
}


void MainWindow::on_actionLaunchInstance_triggered()
{
	doLogin();
}

void MainWindow::doLogin(const QString &errorMsg)
{
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
		tDialog->exec(loginTask);
	}
}

void MainWindow::onLoginComplete(LoginResponse response)
{
	QMessageBox::information(this, "Login Successful", 
							 QString("Logged in as %1 with session ID %2.").
							 arg(response.username(), response.sessionID()));
}

void openInDefaultProgram(QString filename)
{
	QDesktopServices::openUrl("file:///" + QFileInfo(filename).absolutePath());
}
