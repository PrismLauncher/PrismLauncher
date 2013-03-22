#include "consolewindow.h"
#include "ui_consolewindow.h"

#include <QScrollBar>

ConsoleWindow::ConsoleWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConsoleWindow),
	m_mayclose(true)
{
	ui->setupUi(this);
}

ConsoleWindow::~ConsoleWindow()
{
	delete ui;
}

void ConsoleWindow::writeColor(QString text, const char *color)
{
	// append a paragraph
	if (color != nullptr)
		ui->text->appendHtml(QString("<font color=%1>%2</font>").arg(color).arg(text));
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
	QListIterator<QString> iter(paragraphs);
	if (mode == MessageLevel::MultiMC)
		while(iter.hasNext())
			writeColor(iter.next(), "blue");
	else if (mode == MessageLevel::Error)
		while(iter.hasNext())
			writeColor(iter.next(), "red");
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
