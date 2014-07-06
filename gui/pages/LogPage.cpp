#include "LogPage.h"
#include <gui/dialogs/CustomMessageBox.h>
#include <gui/dialogs/ProgressDialog.h>
#include <logic/MinecraftProcess.h>
#include <QtGui/QIcon>
#include "ui_LogPage.h"
#include "logic/net/PasteUpload.h"
#include <QScrollBar>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>

QString LogPage::displayName()
{
	return tr("Minecraft Log");
}

QIcon LogPage::icon()
{
	return QIcon::fromTheme("refresh");
}

QString LogPage::id()
{
	return "console";
}

LogPage::LogPage(MinecraftProcess *proc, QWidget *parent)
	: QWidget(parent), ui(new Ui::LogPage), m_process(proc)
{
	ui->setupUi(this);
	connect(m_process, SIGNAL(log(QString, MessageLevel::Enum)), this,
		SLOT(write(QString, MessageLevel::Enum)));
}

LogPage::~LogPage()
{
	delete ui;
}

bool LogPage::apply()
{
	return true;
}

bool LogPage::shouldDisplay()
{
	return m_process->instance()->isRunning();
}

void LogPage::on_btnPaste_clicked()
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
	else
	{
		QString link = paste->pasteLink();
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(link);
		QDesktopServices::openUrl(link);
		CustomMessageBox::selectable(
			this, tr("Upload finished"),
		tr("The <a href=\"%1\">link to the uploaded log</a> has been opened in the default browser and placed in your clipboard.")
		.arg(link),
		QMessageBox::Information)->exec();
	}
	delete paste;
}

void LogPage::writeColor(QString text, const char *color, const char * background)
{
	// append a paragraph
	QString newtext;
	newtext += "<span style=\"";
	{
		if (color)
			newtext += QString("color:") + color + ";";
		if (background)
			newtext += QString("background-color:") + background + ";";
		newtext += "font-family: monospace;";
	}
	newtext += "\">";
	newtext += text.toHtmlEscaped();
	newtext += "</span>";
	ui->text->appendHtml(newtext);
}

void LogPage::write(QString data, MessageLevel::Enum mode)
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
	QStringList filtered;
	for (QString &paragraph : paragraphs)
	{
		// Quick hack for
		if(paragraph.contains("Detected an attempt by a mod null to perform game activity during mod construction"))
			continue;
		filtered.append(paragraph.trimmed());
	}
	QListIterator<QString> iter(filtered);
	if (mode == MessageLevel::MultiMC)
		while (iter.hasNext())
			writeColor(iter.next(), "blue", 0);
	else if (mode == MessageLevel::Error)
		while (iter.hasNext())
			writeColor(iter.next(), "red", 0);
	else if (mode == MessageLevel::Warning)
		while (iter.hasNext())
			writeColor(iter.next(), "orange", 0);
	else if (mode == MessageLevel::Fatal)
		while (iter.hasNext())
			writeColor(iter.next(), "red", "black");
	else if (mode == MessageLevel::Debug)
		while (iter.hasNext())
			writeColor(iter.next(), "green", 0);
	else if (mode == MessageLevel::PrePost)
		while (iter.hasNext())
			writeColor(iter.next(), "grey", 0);
	// TODO: implement other MessageLevels
	else
		while (iter.hasNext())
			writeColor(iter.next(), 0, 0);
	if(isVisible())
	{
		if (m_scroll_active)
		{
			bar->setValue(bar->maximum());
		}
		m_last_scroll_value = bar->value();
	}
}
