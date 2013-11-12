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

#include <QScrollBar>
#include <QMessageBox>

#include <gui/Platform.h>
#include <gui/dialogs/CustomMessageBox.h>

ConsoleWindow::ConsoleWindow(MinecraftProcess *mcproc, QWidget *parent)
	: QDialog(parent), ui(new Ui::ConsoleWindow), m_mayclose(true), proc(mcproc)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	this->setWindowFlags(Qt::Window);
	connect(mcproc, SIGNAL(ended(BaseInstance *, int, ExitStatus)), this,
			SLOT(onEnded(BaseInstance *, int, QProcess::ExitStatus)));
}

ConsoleWindow::~ConsoleWindow()
{
	delete ui;
}

void ConsoleWindow::writeColor(QString text, const char *color)
{
	// append a paragraph
	if (color != nullptr)
		ui->text->appendHtml(QString("<font color=\"%1\">%2</font>").arg(color).arg(text));
	else
		ui->text->appendPlainText(text);
	// scroll down
	QScrollBar *bar = ui->text->verticalScrollBar();
	bar->setValue(bar->maximum());
}

void ConsoleWindow::write(QString data, MessageLevel::Enum mode)
{
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
	// TODO: implement other MessageLevels
	else
		while (iter.hasNext())
			writeColor(iter.next());
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
	m_mayclose = mayclose;
	if (mayclose)
		ui->closeButton->setEnabled(true);
	else
		ui->closeButton->setEnabled(false);
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
	if (!m_mayclose)
		event->ignore();
	else
		QDialog::closeEvent(event);
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
	ui->btnKillMinecraft->setEnabled(false);

	if (instance->settings().get("AutoCloseConsole").toBool())
	{
		if (code == 0 && status != QProcess::CrashExit)
		{
			this->close();
		}
	}
}
