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

#include "ConsoleWindow.h"
#include "ui_ConsoleWindow.h"
#include "MultiMC.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include <gui/Platform.h>
#include <gui/dialogs/CustomMessageBox.h>
#include <gui/dialogs/ProgressDialog.h>

#include "logic/net/PasteUpload.h"
#include "logic/icons/IconList.h"

ConsoleWindow::ConsoleWindow(MinecraftProcess *mcproc, QWidget *parent)
	: QMainWindow(parent), ui(new Ui::ConsoleWindow), proc(mcproc)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	connect(mcproc, SIGNAL(log(QString, MessageLevel::Enum)), this,
			SLOT(write(QString, MessageLevel::Enum)));
	connect(mcproc, SIGNAL(ended(BaseInstance *, int, QProcess::ExitStatus)), this,
			SLOT(onEnded(BaseInstance *, int, QProcess::ExitStatus)));
	connect(mcproc, SIGNAL(prelaunch_failed(BaseInstance *, int, QProcess::ExitStatus)), this,
			SLOT(onEnded(BaseInstance *, int, QProcess::ExitStatus)));
	connect(mcproc, SIGNAL(launch_failed(BaseInstance *)), this,
			SLOT(onLaunchFailed(BaseInstance *)));

	restoreState(
		QByteArray::fromBase64(MMC->settings()->get("ConsoleWindowState").toByteArray()));
	restoreGeometry(
		QByteArray::fromBase64(MMC->settings()->get("ConsoleWindowGeometry").toByteArray()));

	QString iconKey = proc->instance()->iconKey();
	QString name = proc->instance()->name();
	auto icon = MMC->icons()->getIcon(iconKey);
	setWindowIcon(icon);
	m_trayIcon = new QSystemTrayIcon(icon, this);
	QString consoleTitle = tr("Console window for ") + name;
	m_trayIcon->setToolTip(consoleTitle);
	setWindowTitle(consoleTitle);

	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	m_trayIcon->show();
	if (mcproc->instance()->settings().get("ShowConsole").toBool())
	{
		show();
	}
	setMayClose(false);
}

ConsoleWindow::~ConsoleWindow()
{
	delete ui;
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

void ConsoleWindow::writeColor(QString text, const char *color)
{
	// append a paragraph
	QString newtext;
	newtext += "<span style=\"";
	{
		if (color)
			newtext += QString("color:") + color + ";";
		newtext += "font-family: monospace;";
	}
	newtext += "\">";
	newtext += text.toHtmlEscaped();
	newtext += "</span>";
	ui->text->appendHtml(newtext);
}

void ConsoleWindow::write(QString data, MessageLevel::Enum mode)
{
	QScrollBar *bar = ui->text->verticalScrollBar();
	int max_bar = bar->maximum();
	int val_bar = bar->value();
	if(isVisible())
	{
		if (m_scroll_active)
		{
			m_scroll_active = (max_bar - val_bar) <= 1;
		}
		else
		{
			m_scroll_active = val_bar == max_bar;
		}
	}
	if (data.endsWith('\n'))
		data = data.left(data.length() - 1);
	QStringList paragraphs = data.split('\n');
	for (QString &paragraph : paragraphs)
	{
		paragraph = paragraph.trimmed();
	}

	QListIterator<QString> iter(paragraphs);
	if (mode == MessageLevel::MultiMC)
		while (iter.hasNext())
			writeColor(iter.next(), "blue");
	else if (mode == MessageLevel::Error)
		while (iter.hasNext())
			writeColor(iter.next(), "red");
	else if (mode == MessageLevel::Warning)
		while (iter.hasNext())
			writeColor(iter.next(), "orange");
	else if (mode == MessageLevel::Fatal)
		while (iter.hasNext())
			writeColor(iter.next(), "pink");
	else if (mode == MessageLevel::Debug)
		while (iter.hasNext())
			writeColor(iter.next(), "green");
	else if (mode == MessageLevel::PrePost)
		while (iter.hasNext())
			writeColor(iter.next(), "grey");
	// TODO: implement other MessageLevels
	else
		while (iter.hasNext())
			writeColor(iter.next());
	if(isVisible())
	{
		if (m_scroll_active)
		{
			bar->setValue(bar->maximum());
		}
		m_last_scroll_value = bar->value();
	}
}

void ConsoleWindow::clear()
{
	ui->text->clear();
}

void ConsoleWindow::on_closeButton_clicked()
{
	close();
}

void ConsoleWindow::setMayClose(bool mayclose)
{
	if(mayclose)
		ui->closeButton->setText(tr("Close"));
	else
		ui->closeButton->setText(tr("Hide"));
	m_mayclose = mayclose;
}

void ConsoleWindow::toggleConsole()
{
	QScrollBar *bar = ui->text->verticalScrollBar();
	if (isVisible())
	{
		int max_bar = bar->maximum();
		int val_bar = m_last_scroll_value = bar->value();
		m_scroll_active = (max_bar - val_bar) <= 1;
		hide();
	}
	else
	{
		show();
		if (m_scroll_active)
		{
			bar->setValue(bar->maximum());
		}
		else
		{
			bar->setValue(m_last_scroll_value);
		}
	}
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
	if (!m_mayclose)
	{
		toggleConsole();
	}
	else
	{
		MMC->settings()->set("ConsoleWindowState", saveState().toBase64());
		MMC->settings()->set("ConsoleWindowGeometry", saveGeometry().toBase64());

		emit isClosing();
		m_trayIcon->hide();
		QMainWindow::closeEvent(event);
	}
}

void ConsoleWindow::on_btnKillMinecraft_clicked()
{
	ui->btnKillMinecraft->setEnabled(false);
	auto response = CustomMessageBox::selectable(
		this, tr("Kill Minecraft?"),
		tr("This can cause the instance to get corrupted and should only be used if Minecraft "
		   "is frozen for some reason"),
		QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
	if (response == QMessageBox::Yes)
		proc->killMinecraft();
	else
		ui->btnKillMinecraft->setEnabled(true);
}

void ConsoleWindow::onEnded(BaseInstance *instance, int code, QProcess::ExitStatus status)
{
	bool peacefulExit = code == 0 && status != QProcess::CrashExit;
	ui->btnKillMinecraft->setEnabled(false);

	setMayClose(true);

	if (instance->settings().get("AutoCloseConsole").toBool())
	{
		if (peacefulExit)
		{
			this->close();
			return;
		}
	}
	/*
	if(!peacefulExit)
	{
		m_trayIcon->showMessage(tr("Oh no!"), tr("Minecraft crashed!"), QSystemTrayIcon::Critical);
	}
	*/
	if (!isVisible())
		show();
}

void ConsoleWindow::onLaunchFailed(BaseInstance *instance)
{
	ui->btnKillMinecraft->setEnabled(false);

	setMayClose(true);

	if (!isVisible())
		show();
}

void ConsoleWindow::on_btnPaste_clicked()
{
	auto text = ui->text->toPlainText();
	ProgressDialog dialog(this);
	PasteUpload *paste = new PasteUpload(this, text);
	dialog.exec(paste);
	if (!paste->successful())
	{
		CustomMessageBox::selectable(this, "Upload failed", paste->failReason(),
									 QMessageBox::Critical)->exec();
	}
}
