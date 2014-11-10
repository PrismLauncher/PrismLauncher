#include "LogPage.h"
#include "ui_LogPage.h"

#include "MultiMC.h"

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

	// create the format and set its font
	defaultFormat = new QTextCharFormat(ui->text->currentCharFormat());
	QString fontFamily = MMC->settings()->get("ConsoleFont").toString();
	bool conversionOk = false;
	int fontSize = MMC->settings()->get("ConsoleFontSize").toInt(&conversionOk);
	if(!conversionOk)
	{
		fontSize = 11;
	}
	defaultFormat->setFont(QFont(fontFamily, fontSize));

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
	delete defaultFormat;
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
		filtered.append(paragraph);
	}
	QListIterator<QString> iter(filtered);
	QTextCharFormat format(*defaultFormat);

	switch(mode)
	{
		case MessageLevel::MultiMC:
		{
			format.setForeground(QColor("blue"));
			break;
		}
		case MessageLevel::Debug:
		{
			format.setForeground(QColor("green"));
			break;
		}
		case MessageLevel::Warning:
		{
			format.setForeground(QColor("orange"));
			break;
		}
		case MessageLevel::Error:
		{
			format.setForeground(QColor("red"));
			break;
		}
		case MessageLevel::Fatal:
		{
			format.setForeground(QColor("red"));
			format.setBackground(QColor("black"));
			break;
		}
		case MessageLevel::PrePost:
		{
			format.setForeground(QColor("grey"));
			break;
		}
		case MessageLevel::Info:
		case MessageLevel::Message:
		default:
		{
			// do nothing, keep original
		}
	}

	while (iter.hasNext())
	{
		// append a paragraph/line
		auto workCursor = ui->text->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(iter.next(), format);
		workCursor.insertBlock();
	}

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
