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

#include "util/osutils.h"
#include "util/userutil.h"
#include "util/pathutils.h"

#include "gui/settingsdialog.h"
#include "gui/newinstancedialog.h"
#include "gui/logindialog.h"
#include "gui/taskdialog.h"
#include "gui/browserdialog.h"

#include "data/appsettings.h"
#include "data/version.h"

#include "tasks/logintask.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	setWindowTitle(QString("MultiMC %1").arg(Version::current.toString()));
	
	restoreGeometry(settings->getConfig().value("MainWindowGeometry", saveGeometry()).toByteArray());
	restoreState(settings->getConfig().value("MainWindowState", saveState()).toByteArray());
	
	instList.initialLoad("instances");
	ui->instanceView->setModel(&instList);
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
	openInDefaultProgram(settings->getInstanceDir());
}

void MainWindow::on_actionRefresh_triggered()
{
	instList.initialLoad("instances");
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
	openInDefaultProgram(settings->getCentralModsDir());
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
    //QDesktopServices::openUrl(QUrl("http://bugs.forkk.net/"));
    openWebPage(QUrl("http://bugs.forkk.net/"));
}

void MainWindow::on_actionNews_triggered()
{
    //QDesktopServices::openUrl(QUrl("http://news.forkk.net/"));
    openWebPage(QUrl("http://news.forkk.net/"));
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
	settings->getConfig().setValue("MainWindowGeometry", saveGeometry());
	settings->getConfig().setValue("MainWindowState", saveState());
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
							 arg(response.getUsername(), response.getSessionID()));
}

// Create A Desktop Shortcut
void MainWindow::on_actionMakeDesktopShortcut_triggered()
{
    QString name("Test");
    name = QInputDialog::getText(this, tr("MultiMC Shortcut"), tr("Enter a Shortcut Name."), QLineEdit::Normal, name);

    Util::createShortCut(Util::getDesktopDir(), QApplication::instance()->applicationFilePath(), QStringList() << "-dl" << QDir::currentPath() << "test", name, "application-x-octet-stream");

    QMessageBox::warning(this, "Not useful", "A Dummy Shortcut was created. it will not do anything productive");
}

// BrowserDialog
void MainWindow::openWebPage(QUrl url)
{
    BrowserDialog *browser = new BrowserDialog(this);

    browser->load(url);
    browser->exec();
}
