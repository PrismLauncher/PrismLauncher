#include "consolewindow.h"
#include "ui_consolewindow.h"

#include <QScrollBar>
#include <QMessageBox>

ConsoleWindow::ConsoleWindow(MinecraftProcess *mcproc, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConsoleWindow),
	m_mayclose(true),
	proc(mcproc)
{
	ui->setupUi(this);
	connect(mcproc, SIGNAL(ended()), this, SLOT(onEnded()));
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
	QMessageBox r_u_sure;
	//: Main question of the kill confirmation dialog
	r_u_sure.setText(tr("Kill Minecraft?"));
	r_u_sure.setInformativeText(tr("This can cause the instance to get corrupted and should only be used if Minecraft is frozen for some reason"));
	r_u_sure.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	r_u_sure.setDefaultButton(QMessageBox::Yes);
	if (r_u_sure.exec() == QMessageBox::Yes)
		proc->killMinecraft();
	else
		ui->btnKillMinecraft->setEnabled(true);
	r_u_sure.close();
}

void ConsoleWindow::onEnded()
{
	ui->btnKillMinecraft->setEnabled(false);
	// TODO: Check why this doesn't work
	if (!proc->exitCode()) this->close(); 
}
