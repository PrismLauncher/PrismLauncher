/* Copyright 2013-2015 MultiMC Contributors
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

#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include "BaseLauncher.h"

class QPushButton;
class PageContainer;
class ConsoleWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit ConsoleWindow(std::shared_ptr<BaseLauncher> proc, QWidget *parent = 0);
	virtual ~ConsoleWindow();

	/**
	 * @brief specify if the window is allowed to close
	 * @param mayclose
	 * used to keep it alive while MC runs
	 */
	void setMayClose(bool mayclose);

signals:
	void isClosing();

private
slots:
	void on_closeButton_clicked();
	void on_btnKillMinecraft_clicked();

	void onEnded(InstancePtr instance, int code, QProcess::ExitStatus status);
	void onLaunchFailed(InstancePtr instance);

	// FIXME: add handlers for the other MinecraftLauncher signals (pre/post launch command
	// failures)

	void iconActivated(QSystemTrayIcon::ActivationReason);
	void toggleConsole();
protected:
	void closeEvent(QCloseEvent *);

private:
	std::shared_ptr<BaseLauncher> m_proc;
	bool m_mayclose = true;
	QSystemTrayIcon *m_trayIcon = nullptr;
	PageContainer *m_container = nullptr;
	QPushButton *m_closeButton = nullptr;
	QPushButton *m_killButton = nullptr;
};
