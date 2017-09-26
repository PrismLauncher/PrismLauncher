/* Copyright 2013-2017 MultiMC Contributors
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
		m_container->setParentContainer(this);
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
		connect(m_killButton, SIGNAL(clicked(bool)), SLOT(on_btnKillMinecraft_clicked()));

		m_launchOfflineButton = new QPushButton();
		horizontalLayout->addWidget(m_launchOfflineButton);
		m_launchOfflineButton->setText(tr("Launch Offline"));
		updateLaunchButtons();
		connect(m_launchOfflineButton, SIGNAL(clicked(bool)), SLOT(on_btnLaunchMinecraftOffline_clicked()));

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
		connect(m_instance.get(), &BaseInstance::launchTaskChanged, this, &InstanceWindow::on_InstanceLaunchTask_changed);
		connect(m_instance.get(), &BaseInstance::runningStatusChanged, this, &InstanceWindow::on_RunningState_changed);
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

void InstanceWindow::updateLaunchButtons()
{
	if(m_instance->isRunning())
	{
		m_launchOfflineButton->setEnabled(false);
		m_killButton->setText(tr("Kill"));
		m_killButton->setToolTip(tr("Kill the running instance"));
	}
	else if(!m_instance->canLaunch())
	{
		m_launchOfflineButton->setEnabled(false);
		m_killButton->setText(tr("Launch"));
		m_killButton->setToolTip(tr("Launch the instance"));
		m_killButton->setEnabled(false);
	}
	else
	{
		m_launchOfflineButton->setEnabled(true);
		m_killButton->setText(tr("Launch"));
		m_killButton->setToolTip(tr("Launch the instance"));
	}
}

void InstanceWindow::on_btnLaunchMinecraftOffline_clicked()
{
	MMC->launch(m_instance, false, nullptr);
}

void InstanceWindow::on_InstanceLaunchTask_changed(std::shared_ptr<LaunchTask> proc)
{
	m_proc = proc;
}

void InstanceWindow::on_RunningState_changed(bool)
{
	updateLaunchButtons();
	m_container->refreshContainer();
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
		MMC->kill(m_instance);
	}
	else
	{
		MMC->launch(m_instance, true, nullptr);
	}
}

QString InstanceWindow::instanceId()
{
	return m_instance->id();
}

bool InstanceWindow::selectPage(QString pageId)
{
	return m_container->selectPage(pageId);
}

void InstanceWindow::refreshContainer()
{
	m_container->refreshContainer();
}

InstanceWindow::~InstanceWindow()
{
}

bool InstanceWindow::requestClose()
{
	if(m_container->prepareToClose())
	{
		close();
		return true;
	}
	return false;
}
