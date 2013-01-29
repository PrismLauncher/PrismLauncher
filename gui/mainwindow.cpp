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

#include <QDesktopServices>
#include <QUrl>

#include "util/osutils.h"
#include "gui/settingsdialog.h"
#include "data/appsettings.h"
#include "data/version.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	setWindowTitle(QString("MultiMC %1").arg(Version::current.toString()));
	
	instList.initialLoad("instances");
	ui->instanceView->setModel(&instList);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_actionAddInstance_triggered()
{
	
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
	
}

void MainWindow::on_actionNews_triggered()
{
	
}

void MainWindow::on_actionAbout_triggered()
{
	
}
