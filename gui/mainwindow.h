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

#include "lists/InstanceList.h"
#include "tasks/LoginTask.h"
#include "BaseInstance.h"

class InstanceModel;
class InstanceProxyModel;
class KCategorizedView;
class KCategoryDrawer;
class MinecraftProcess;
class ConsoleWindow;
class OneSixAssets;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
	/*!
	 * The currently selected instance.
	 */
	Q_PROPERTY(BaseInstance* selectedInstance READ selectedInstance STORED false)
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
	void closeEvent(QCloseEvent *event);

	// Browser Dialog
	void openWebPage(QUrl url);
	
	
	BaseInstance *selectedInstance();
	
private slots:
	void on_actionAbout_triggered();
	
	void on_actionAddInstance_triggered();
	
	void on_actionChangeInstGroup_triggered();
	
	void on_actionChangeInstIcon_triggered();
	
	void on_actionViewInstanceFolder_triggered();
	
	void on_actionViewSelectedInstFolder_triggered();

	void on_actionRefresh_triggered();
	
	void on_actionViewCentralModsFolder_triggered();
	
	void on_actionCheckUpdate_triggered();
	
	void on_actionSettings_triggered();
	
	void on_actionReportBug_triggered();
	
	void on_actionNews_triggered();
	
	void on_mainToolBar_visibilityChanged(bool);
	
	void on_instanceView_customContextMenuRequested(const QPoint &pos);
	
	void on_actionLaunchInstance_triggered();
	
	void on_actionDeleteInstance_triggered();
	
	void on_actionRenameInstance_triggered();
	
	void on_actionMakeDesktopShortcut_triggered();
	
	void on_actionChangeInstMCVersion_triggered();
	
	void on_actionEditInstMods_triggered();
	
	void doLogin(const QString& errorMsg = "");
	
	
	void onLoginComplete();
	
	
	void onGameUpdateComplete();
	void onGameUpdateError(QString error);
	
	void taskStart();
	void taskEnd();

	void on_actionChangeInstLWJGLVersion_triggered();
	
    void on_actionInstanceSettings_triggered();

public slots:
	void instanceActivated ( QModelIndex );

	void instanceChanged ( QModelIndex );
	
	void startTask(Task *task);
	
	void launchInstance(BaseInstance *inst, LoginResponse response);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	
private:
	Ui::MainWindow *ui;
	KCategoryDrawer * drawer;
	KCategorizedView * view;
	InstanceModel * model;
	InstanceProxyModel * proxymodel;
	InstanceList instList;
	MinecraftProcess *proc;
	ConsoleWindow *console;
	OneSixAssets *assets_downloader;
	
	// A pointer to the instance we are actively doing stuff with.
	// This is set when the user launches an instance and is used to refer to that
	// instance throughout the launching process.
	BaseInstance *m_activeInst;
	LoginResponse m_activeLogin;
	
	Task *m_versionLoadTask;
};

#endif // MAINWINDOW_H
