#include "LogPage.h"
#include "ui_LogPage.h"

#include "MultiMC.h"

#include <QIcon>
#include <QScrollBar>
#include <QShortcut>

#include "launch/LaunchTask.h"
#include <settings/Setting.h>
#include "GuiUtil.h"
#include <ColorCache.h>

LogPage::LogPage(InstancePtr instance, QWidget *parent)
	: QWidget(parent), ui(new Ui::LogPage), m_instance(instance)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	// create the format and set its font
	{
		defaultFormat = new QTextCharFormat(ui->text->currentCharFormat());
		QString fontFamily = MMC->settings()->get("ConsoleFont").toString();
		bool conversionOk = false;
		int fontSize = MMC->settings()->get("ConsoleFontSize").toInt(&conversionOk);
		if(!conversionOk)
		{
			fontSize = 11;
		}
		defaultFormat->setFont(QFont(fontFamily, fontSize));
	}

	// ensure we don't eat all the RAM
	{
		auto lineSetting = MMC->settings()->getSetting("ConsoleMaxLines");
		bool conversionOk = false;
		int maxLines = lineSetting->get().toInt(&conversionOk);
		if(!conversionOk)
		{
			maxLines = lineSetting->defValue().toInt();
			qWarning() << "ConsoleMaxLines has nonsensical value, defaulting to" << maxLines;
		}
		ui->text->setMaximumBlockCount(maxLines);

		m_stopOnOverflow = MMC->settings()->get("ConsoleOverflowStop").toBool();
	}

	// set up instance and launch process recognition
	{
		auto launchTask = m_instance->getLaunchTask();
		if(launchTask)
		{
			on_InstanceLaunchTask_changed(launchTask);
		}
		connect(m_instance.get(), &BaseInstance::launchTaskChanged,
			this, &LogPage::on_InstanceLaunchTask_changed);
	}

	// set up text colors and adapt them to the current theme foreground and background
	{
		auto origForeground = ui->text->palette().color(ui->text->foregroundRole());
		auto origBackground = ui->text->palette().color(ui->text->backgroundRole());
		m_colors.reset(new LogColorCache(origForeground, origBackground));
	}

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

void LogPage::on_InstanceLaunchTask_changed(std::shared_ptr<LaunchTask> proc)
{
	if(m_process)
	{
		disconnect(m_process.get(), &LaunchTask::log, this, &LogPage::write);
	}
	m_process = proc;
	if(m_process)
	{
		ui->text->clear();
		connect(m_process.get(), &LaunchTask::log, this, &LogPage::write);
	}
}

bool LogPage::apply()
{
	return true;
}

bool LogPage::shouldDisplay() const
{
	return m_instance->isRunning() || ui->text->blockCount() > 1;
}

void LogPage::on_btnPaste_clicked()
{
	//FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
	write(tr("MultiMC: Log upload triggered at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)), MessageLevel::MultiMC);
	auto url = GuiUtil::uploadPaste(ui->text->toPlainText(), this);
	if(!url.isEmpty())
	{
		write(tr("MultiMC: Log uploaded to: %1").arg(url), MessageLevel::MultiMC);
	}
	else
	{
		write(tr("MultiMC: Log upload failed!"), MessageLevel::Error);
	}
}

void LogPage::on_btnCopy_clicked()
{
	write(QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)), MessageLevel::MultiMC);
	GuiUtil::setClipboardText(ui->text->toPlainText());
}

void LogPage::on_btnClear_clicked()
{
	ui->text->clear();
}

void LogPage::on_btnBottom_clicked()
{
	ui->text->verticalScrollBar()->setSliderPosition(ui->text->verticalScrollBar()->maximum());
}

void LogPage::on_trackLogCheckbox_clicked(bool checked)
{
	m_write_active = checked;
}

void LogPage::on_wrapCheckbox_clicked(bool checked)
{
	if(checked)
	{
		ui->text->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	}
	else
	{
		ui->text->setWordWrapMode(QTextOption::WrapMode::NoWrap);
	}
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

void LogPage::setParentContainer(BasePageContainer * container)
{
	m_parentContainer = container;
}

void LogPage::write(QString data, MessageLevel::Enum mode)
{
	if (!m_write_active)
	{
		if (mode != MessageLevel::MultiMC)
		{
			return;
		}
	}
	if(m_stopOnOverflow && m_write_active)
	{
		if(mode != MessageLevel::MultiMC)
		{
			if(ui->text->blockCount() >= ui->text->maximumBlockCount())
			{
				m_write_active = false;
				data = tr("MultiMC stopped watching the game log because the log length surpassed %1 lines.\n"
					"You may have to fix your mods because the game is still loggging to files and"
						" likely wasting harddrive space at an alarming rate!")
							.arg(ui->text->maximumBlockCount());
				mode = MessageLevel::Fatal;
				ui->trackLogCheckbox->setCheckState(Qt::Unchecked);
				if(!isVisible())
				{
					m_parentContainer->selectPage(id());
				}
			}
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

	format.setForeground(m_colors->getFront(mode));
	format.setBackground(m_colors->getBack(mode));

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
