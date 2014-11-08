#include "LogPage.h"
#include "ui_LogPage.h"

#include <QIcon>
#include <QScrollBar>
#include <QShortcut>

#include "logic/MinecraftProcess.h"
#include "gui/GuiUtil.h"

LogPage::LogPage(MinecraftProcess *proc, QWidget *parent)
	: QWidget(parent), ui(new Ui::LogPage), m_process(proc)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	connect(m_process, SIGNAL(log(QString, MessageLevel::Enum)), this,
			SLOT(write(QString, MessageLevel::Enum)));

	auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
	connect(findShortcut, SIGNAL(activated()), SLOT(findActivated()));
	auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
	connect(findNextShortcut, SIGNAL(activated()), SLOT(findNextActivated()));
	connect(ui->searchBar, SIGNAL(returnPressed()), SLOT(on_findButton_clicked()));
	auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
	connect(findPreviousShortcut, SIGNAL(activated()), SLOT(findPreviousActivated()));
}

LogPage::~LogPage()
{
	delete ui;
}

bool LogPage::apply()
{
	return true;
}

bool LogPage::shouldDisplay() const
{
	return m_process->instance()->isRunning();
}

void LogPage::on_btnPaste_clicked()
{
	GuiUtil::uploadPaste(ui->text->toPlainText(), this);
}

void LogPage::on_btnCopy_clicked()
{
	GuiUtil::setClipboardText(ui->text->toPlainText());
}

void LogPage::on_btnClear_clicked()
{
	ui->text->clear();
}

void LogPage::on_trackLogCheckbox_clicked(bool checked)
{
	m_write_active = checked;
}

void LogPage::on_findButton_clicked()
{
	auto modifiers = QApplication::keyboardModifiers();
	if (modifiers & Qt::ShiftModifier)
	{
		findPreviousActivated();
	}
	else
	{
		findNextActivated();
	}
}

void LogPage::findActivated()
{
	// focus the search bar if it doesn't have focus
	if (!ui->searchBar->hasFocus())
	{
		auto searchForCursor = ui->text->textCursor();
		auto searchForString = searchForCursor.selectedText();
		if (searchForString.size())
		{
			ui->searchBar->setText(searchForString);
		}
		ui->searchBar->setFocus();
		ui->searchBar->selectAll();
	}
}

void LogPage::findNextActivated()
{
	auto toSearch = ui->searchBar->text();
	if (toSearch.size())
	{
		ui->text->find(toSearch);
	}
}

void LogPage::findPreviousActivated()
{
	auto toSearch = ui->searchBar->text();
	if (toSearch.size())
	{
		ui->text->find(toSearch, QTextDocument::FindBackward);
	}
}

void LogPage::writeColor(QString text, const char *color, const char *background)
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
	if (!m_write_active)
	{
		if (mode != MessageLevel::PrePost && mode != MessageLevel::MultiMC)
		{
			return;
		}
	}

	// save the cursor so it can be restored.
	auto savedCursor = ui->text->cursor();

	QScrollBar *bar = ui->text->verticalScrollBar();
	int max_bar = bar->maximum();
	int val_bar = bar->value();
	if (isVisible())
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
		//TODO: implement filtering here.
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
	if (isVisible())
	{
		if (m_scroll_active)
		{
			bar->setValue(bar->maximum());
		}
		m_last_scroll_value = bar->value();
	}
	ui->text->setCursor(savedCursor);
}
