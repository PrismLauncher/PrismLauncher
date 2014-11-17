/* Copyright 2013-2014 MultiMC Contributors
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

#include "ConsoleWindow.h"
#include "MultiMC.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QHBoxLayout>
#include <QPushButton>
#include <qlayoutitem.h>
#include <QCloseEvent>

#include <gui/Platform.h>
#include <gui/dialogs/CustomMessageBox.h>
#include <gui/dialogs/ProgressDialog.h>
#include "widgets/PageContainer.h"
#include "pages/LogPage.h"

#include "logic/icons/IconList.h"

class LogPageProvider : public BasePageProvider
{
public:
	LogPageProvider(BasePageProviderPtr parent, BasePage * log_page)
	{
		m_parent = parent;
		m_log_page = log_page;
	}
	virtual QString dialogTitle() {return "Fake";};
	virtual QList<BasePage *> getPages()
	{
		auto pages = m_parent->getPages();
		pages.prepend(m_log_page);
		return pages;
	}
private:
	BasePageProviderPtr m_parent;
	BasePage * m_log_page;
};

ConsoleWindow::ConsoleWindow(MinecraftProcess *mcproc, QWidget *parent)
	: QMainWindow(parent), m_proc(mcproc)
{
	MultiMCPlatform::fixWM_CLASS(this);
	setAttribute(Qt::WA_DeleteOnClose);

	auto instance = m_proc->instance();
	auto icon = MMC->icons()->getIcon(instance->iconKey());
	QString windowTitle = tr("Console window for ") + instance->name();

	// Set window properties
	{
		setWindowIcon(icon);
		setWindowTitle(windowTitle);
	}

	// Add page container
	{
		auto mainLayout = new QVBoxLayout;
		auto provider = std::dynamic_pointer_cast<BasePageProvider>(m_proc->instance());
		auto proxy_provider = std::make_shared<LogPageProvider>(provider, new LogPage(m_proc));
		m_container = new PageContainer(proxy_provider, "console", this);
		mainLayout->addWidget(m_container);
		mainLayout->setSpacing(0);
		mainLayout->setContentsMargins(0,0,0,0);
		setLayout(mainLayout);
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
		m_killButton->setText(tr("Kill Minecraft"));
		horizontalLayout->addWidget(m_killButton);
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

	// Set up tray icon
	{
		m_trayIcon = new QSystemTrayIcon(icon, this);
		m_trayIcon->setToolTip(windowTitle);
		
		connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
				SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
		m_trayIcon->show();
	}

	// Set up signal connections
	connect(mcproc, SIGNAL(ended(InstancePtr, int, QProcess::ExitStatus)), this,
			SLOT(onEnded(InstancePtr, int, QProcess::ExitStatus)));
	connect(mcproc, SIGNAL(prelaunch_failed(InstancePtr, int, QProcess::ExitStatus)), this,
			SLOT(onEnded(InstancePtr, int, QProcess::ExitStatus)));
	connect(mcproc, SIGNAL(launch_failed(InstancePtr)), this,
			SLOT(onLaunchFailed(InstancePtr)));

	setMayClose(false);

	if (mcproc->instance()->settings().get("ShowConsole").toBool())
	{
		show();
	}
}

void ConsoleWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
	case QSystemTrayIcon::Trigger:
	{
		toggleConsole();
	}
	default:
		return;
	}
}

void ConsoleWindow::on_closeButton_clicked()
{
	close();
}

void ConsoleWindow::setMayClose(bool mayclose)
{
	if(mayclose)
		m_closeButton->setText(tr("Close"));
	else
		m_closeButton->setText(tr("Hide"));
	m_mayclose = mayclose;
}

void ConsoleWindow::toggleConsole()
{
	if (isVisible())
	{
		if(!isActiveWindow())
		{
			activateWindow();
			return;
		}
		hide();
	}
	else
	{
		show();
	}
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
	if (!m_mayclose)
	{
		toggleConsole();
		event->ignore();
	}
	else if(m_container->requestClose(event))
	{
		MMC->settings()->set("ConsoleWindowState", saveState().toBase64());
		MMC->settings()->set("ConsoleWindowGeometry", saveGeometry().toBase64());

		emit isClosing();
		m_trayIcon->hide();
		event->accept();
	}
}

void ConsoleWindow::on_btnKillMinecraft_clicked()
{
	m_killButton->setEnabled(false);
	auto response = CustomMessageBox::selectable(
		this, tr("Kill Minecraft?"),
		tr("This can cause the instance to get corrupted and should only be used if Minecraft "
		   "is frozen for some reason"),
		QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
	if (response == QMessageBox::Yes)
		m_proc->killMinecraft();
	else
		m_killButton->setEnabled(true);
}

void ConsoleWindow::onEnded(InstancePtr instance, int code, QProcess::ExitStatus status)
{
	bool peacefulExit = code == 0 && status != QProcess::CrashExit;
	m_killButton->setEnabled(false);
	setMayClose(true);
	if (instance->settings().get("AutoCloseConsole").toBool())
	{
		if (peacefulExit)
		{
			this->close();
			return;
		}
	}
	if (!isVisible())
	{
		show();
	}
	// Raise Window
	if (MMC->settings()->get("RaiseConsole").toBool())
	{
		raise();
		activateWindow();
	}
}

void ConsoleWindow::onLaunchFailed(InstancePtr instance)
{
	m_killButton->setEnabled(false);

	setMayClose(true);

	if (!isVisible())
		show();
}
ConsoleWindow::~ConsoleWindow()
{
	
}
