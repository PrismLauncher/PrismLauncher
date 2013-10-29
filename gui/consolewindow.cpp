#include "consolewindow.h"
#include "ui_consolewindow.h"

#include <QScrollBar>
#include <QMessageBox>

#include <gui/platform.h>
#include <gui/CustomMessageBox.h>

ConsoleWindow::ConsoleWindow(MinecraftProcess *mcproc, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConsoleWindow),
	m_mayclose(true),
	proc(mcproc)
{
    MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	connect(mcproc, SIGNAL(ended(BaseInstance*)), this, SLOT(onEnded(BaseInstance*)));
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
		data = data.left(data.length()-1);
	QStringList paragraphs = data.split('\n');
	for(QString &paragraph : paragraphs)
	{
		paragraph = paragraph.trimmed();
	}

	QListIterator<QString> iter(paragraphs);
	if (mode == MessageLevel::MultiMC)
		while(iter.hasNext())
			writeColor(iter.next(), "blue");
	else if (mode == MessageLevel::Error)
		while(iter.hasNext())
			writeColor(iter.next(), "red");
	else if (mode == MessageLevel::Warning)
		while(iter.hasNext())
			writeColor(iter.next(), "orange");
	else if (mode == MessageLevel::Fatal)
		while(iter.hasNext())
			writeColor(iter.next(), "pink");
	else if (mode == MessageLevel::Debug)
		while(iter.hasNext())
			writeColor(iter.next(), "green");
	// TODO: implement other MessageLevels
	else
		while(iter.hasNext())
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

void ConsoleWindow::closeEvent(QCloseEvent * event)
{
	if(!m_mayclose)
		event->ignore();
	else
		QDialog::closeEvent(event);
}

void ConsoleWindow::on_btnKillMinecraft_clicked()
{
	ui->btnKillMinecraft->setEnabled(false);
	auto response = CustomMessageBox::selectable(this, tr("Kill Minecraft?"),
												 tr("This can cause the instance to get corrupted and should only be used if Minecraft is frozen for some reason"),
												 QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
	if (response == QMessageBox::Yes)
		proc->killMinecraft();
	else
		ui->btnKillMinecraft->setEnabled(true);
}

void ConsoleWindow::onEnded(BaseInstance *instance)
{
	ui->btnKillMinecraft->setEnabled(false);

	// TODO: Might need an option to forcefully close, even on an error
	if(instance->settings().get("AutoCloseConsole").toBool())
	{
		// TODO: Check why this doesn't work
		if (!proc->exitCode()) this->close();
	}
}
