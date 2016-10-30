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

#include "InstanceWindow.h"
#include "MultiMC.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <qlayoutitem.h>
#include <QCloseEvent>

#include <dialogs/CustomMessageBox.h>
#include <dialogs/ProgressDialog.h>
#include "widgets/PageContainer.h"
#include "InstancePageProvider.h"

#include "icons/IconList.h"

InstanceWindow::InstanceWindow(InstancePtr instance, QWidget *parent)
	: QMainWindow(parent), m_instance(instance)
{
	setAttribute(Qt::WA_DeleteOnClose);

	auto icon = MMC->icons()->getIcon(m_instance->iconKey());
	QString windowTitle = tr("Console window for ") + m_instance->name();

	// Set window properties
	{
		setWindowIcon(icon);
		setWindowTitle(windowTitle);
	}

	// Add page container
	{
		auto provider = std::make_shared<InstancePageProvider>(m_instance);
		m_container = new PageContainer(provider, "console", this);
		setCentralWidget(m_container);
	}

	// Add custom buttons to the page container layout.
	{
		auto horizontalLayout = new QHBoxLayout();
		horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
		horizontalLayout->setContentsMargins(6, -1, 6, -1);

		auto btnHelp = new QPushButton();
		btnHelp->setText(tr("Help"));
		horizontalLayout->addWidget(btnHelp);
		connect(btnHelp, SIGNAL(clicked(bool)), m_container, SLOT(help()));

		auto spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
		horizontalLayout->addSpacerItem(spacer);

		m_killButton = new QPushButton();
		horizontalLayout->addWidget(m_killButton);
		setKillButton(m_instance->isRunning());
		connect(m_killButton, SIGNAL(clicked(bool)), SLOT(on_btnKillMinecraft_clicked()));

		m_closeButton = new QPushButton();
		m_closeButton->setText(tr("Close"));
		horizontalLayout->addWidget(m_closeButton);
		connect(m_closeButton, SIGNAL(clicked(bool)), SLOT(on_closeButton_clicked()));

		m_container->addButtons(horizontalLayout);
	}

	// restore window state
	{
		auto base64State = MMC->settings()->get("ConsoleWindowState").toByteArray();
		restoreState(QByteArray::fromBase64(base64State));
		auto base64Geometry = MMC->settings()->get("ConsoleWindowGeometry").toByteArray();
		restoreGeometry(QByteArray::fromBase64(base64Geometry));
	}

	// set up instance and launch process recognition
	{
		auto launchTask = m_instance->getLaunchTask();
		on_InstanceLaunchTask_changed(launchTask);
		connect(m_instance.get(), &BaseInstance::launchTaskChanged,
			this, &InstanceWindow::on_InstanceLaunchTask_changed);
		connect(m_instance.get(), &BaseInstance::runningStatusChanged,
			this, &InstanceWindow::on_RunningState_changed);
	}

	// set up instance destruction detection
	{
		connect(m_instance.get(), &BaseInstance::statusChanged, this, &InstanceWindow::on_instanceStatusChanged);
	}
	show();
}

void InstanceWindow::on_instanceStatusChanged(BaseInstance::Status, BaseInstance::Status newStatus)
{
	if(newStatus == BaseInstance::Status::Gone)
	{
		m_doNotSave = true;
		close();
	}
}

void InstanceWindow::setKillButton(bool kill)
{
	if(kill)
	{
		m_killButton->setText(tr("Kill"));
		m_killButton->setToolTip(tr("Kill the running instance"));
	}
	else
	{
		m_killButton->setText(tr("Launch"));
		m_killButton->setToolTip(tr("Launch the instance"));
	}
}

void InstanceWindow::on_InstanceLaunchTask_changed(std::shared_ptr<LaunchTask> proc)
{
	if(m_proc)
	{
		disconnect(m_proc.get(), &LaunchTask::succeeded, this, &InstanceWindow::onSucceeded);
		disconnect(m_proc.get(), &LaunchTask::failed, this,  &InstanceWindow::onFailed);
		disconnect(m_proc.get(), &LaunchTask::requestProgress, this, &InstanceWindow::onProgressRequested);
	}

	m_proc = proc;

	if(m_proc)
	{
		// Set up signal connections
		connect(m_proc.get(), &LaunchTask::succeeded, this, &InstanceWindow::onSucceeded);
		connect(m_proc.get(), &LaunchTask::failed, this,  &InstanceWindow::onFailed);
		connect(m_proc.get(), &LaunchTask::requestProgress, this, &InstanceWindow::onProgressRequested);
	}
}

void InstanceWindow::on_RunningState_changed(bool running)
{
	setKillButton(running);
	m_container->refresh();
}

void InstanceWindow::on_closeButton_clicked()
{
	close();
}

void InstanceWindow::closeEvent(QCloseEvent *event)
{
	bool proceed = true;
	if(!m_doNotSave)
	{
		proceed &= m_container->prepareToClose();
	}

	if(!proceed)
	{
		return;
	}

	MMC->settings()->set("ConsoleWindowState", saveState().toBase64());
	MMC->settings()->set("ConsoleWindowGeometry", saveGeometry().toBase64());
	emit isClosing();
	event->accept();
}

bool InstanceWindow::saveAll()
{
	return m_container->prepareToClose();
}

void InstanceWindow::on_btnKillMinecraft_clicked()
{
	if(m_instance->isRunning())
	{
		auto response = CustomMessageBox::selectable(
			this, tr("Kill Minecraft?"),
			tr("This can cause the instance to get corrupted and should only be used if Minecraft "
			"is frozen for some reason"),
			QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
		if (response == QMessageBox::Yes)
		{
			m_proc->abort();
		}
	}
	// FIXME: duplicate logic between MainWindow and InstanceWindow
	else if(saveAll())
	{
		MMC->launch(m_instance, true, nullptr);
	}
}

void InstanceWindow::onSucceeded()
{
	if (m_instance->settings()->get("AutoCloseConsole").toBool() && m_container->prepareToClose())
	{
		this->close();
		return;
	}
	// Raise Window
	if (MMC->settings()->get("RaiseConsole").toBool())
	{
		show();
		raise();
		activateWindow();
	}
}

void InstanceWindow::onFailed(QString reason)
{
}

void InstanceWindow::onProgressRequested(Task* task)
{
	ProgressDialog progDialog(this);
	progDialog.setSkipButton(true, tr("Abort"));
	m_proc->proceed();
	progDialog.execWithTask(task);
}

QString InstanceWindow::instanceId()
{
	return m_instance->id();
}

bool InstanceWindow::selectPage(QString pageId)
{
	return m_container->selectPage(pageId);
}

InstanceWindow::~InstanceWindow()
{
}
