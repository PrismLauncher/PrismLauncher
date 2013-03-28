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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "instancelist.h"
#include "loginresponse.h"
#include "instance.h"

class InstanceModel;
class InstanceProxyModel;
class KCategorizedView;
class KCategoryDrawer;
class MinecraftProcess;
class ConsoleWindow;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
	void closeEvent(QCloseEvent *event);

    // Browser Dialog
    void openWebPage(QUrl url);

private:
	Instance *selectedInstance();
	
private slots:
	void on_actionAbout_triggered();
	
	void on_actionAddInstance_triggered();
	
	void on_actionChangeInstGroup_triggered();
	
	void on_actionViewInstanceFolder_triggered();

	void on_actionRefresh_triggered();
	
	void on_actionViewCentralModsFolder_triggered();
	
	void on_actionCheckUpdate_triggered();
	
	void on_actionSettings_triggered();
	
	void on_actionReportBug_triggered();
	
	void on_actionNews_triggered();
	
	void on_mainToolBar_visibilityChanged(bool);
	
	void on_instanceView_customContextMenuRequested(const QPoint &pos);
	
	void on_actionLaunchInstance_triggered();
	

    void on_actionMakeDesktopShortcut_triggered();
	
	void on_actionChangeInstMCVersion_triggered();
	
	void doLogin( QString inst, const QString& errorMsg = "" );
	
	
	void onLoginComplete( QString inst, LoginResponse response );
	void onLoginFailed( QString inst, const QString& errorMsg );

public slots:
	void instanceActivated ( QModelIndex );

private:
	Ui::MainWindow *ui;
	KCategoryDrawer * drawer;
	KCategorizedView * view;
	InstanceModel * model;
	InstanceProxyModel * proxymodel;
	InstanceList instList;
	MinecraftProcess *proc;
	ConsoleWindow *console;
};

#endif // MAINWINDOW_H
