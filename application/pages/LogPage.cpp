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

class LogFormatProxyModel : public QIdentityProxyModel
{
public:
	LogFormatProxyModel(QObject* parent = nullptr) : QIdentityProxyModel(parent)
	{
	}
	QVariant data(const QModelIndex &index, int role) const override
	{
		switch(role)
		{
			case Qt::FontRole:
				return m_font;
			case Qt::TextColorRole:
			{
				MessageLevel::Enum level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
				return m_colors->getFront(level);
			}
			case Qt::BackgroundRole:
			{
				MessageLevel::Enum level = (MessageLevel::Enum) QIdentityProxyModel::data(index, LogModel::LevelRole).toInt();
				return m_colors->getBack(level);
			}
			default:
				return QIdentityProxyModel::data(index, role);
			}
	}

	void setFont(QFont font)
	{
		m_font = font;
	}

	void setColors(LogColorCache* colors)
	{
		m_colors.reset(colors);
	}

	QModelIndex find(const QModelIndex &start, const QString &value, bool reverse) const
	{
		QModelIndex parentIndex = parent(start);
		auto compare = [&](int r) -> QModelIndex
		{
			QModelIndex idx = index(r, start.column(), parentIndex);
			if (!idx.isValid() || idx == start)
			{
				return QModelIndex();
			}
			QVariant v = data(idx, Qt::DisplayRole);
			QString t = v.toString();
			if (t.contains(value, Qt::CaseInsensitive))
				return idx;
			return QModelIndex();
		};
		if(reverse)
		{
			int from = start.row();
			int to = 0;

			for (int i = 0; i < 2; ++i)
			{
				for (int r = from; (r >= to); --r)
				{
					auto idx = compare(r);
					if(idx.isValid())
						return idx;
				}
				// prepare for the next iteration
				from = rowCount() - 1;
				to = start.row();
			}
		}
		else
		{
			int from = start.row();
			int to = rowCount(parentIndex);

			for (int i = 0; i < 2; ++i)
			{
				for (int r = from; (r < to); ++r)
				{
					auto idx = compare(r);
					if(idx.isValid())
						return idx;
				}
				// prepare for the next iteration
				from = 0;
				to = start.row();
			}
		}
		return QModelIndex();
	}
private:
	QFont m_font;
	std::unique_ptr<LogColorCache> m_colors;
};

LogPage::LogPage(InstancePtr instance, QWidget *parent)
	: QWidget(parent), ui(new Ui::LogPage), m_instance(instance)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	m_proxy = new LogFormatProxyModel(this);
	connect(m_proxy, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogPage::rowsAboutToBeInserted);
	connect(m_proxy, &QAbstractItemModel::rowsInserted, this, &LogPage::rowsInserted);

	ui->textView->setModel(m_proxy);

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

	// set up text colors in the log proxy and adapt them to the current theme foreground and background
	{
		auto origForeground = ui->textView->palette().color(ui->textView->foregroundRole());
		auto origBackground = ui->textView->palette().color(ui->textView->backgroundRole());
		m_proxy->setColors(new LogColorCache(origForeground, origBackground));
	}

	// set up fonts in the log proxy
	{
		QString fontFamily = MMC->settings()->get("ConsoleFont").toString();
		bool conversionOk = false;
		int fontSize = MMC->settings()->get("ConsoleFontSize").toInt(&conversionOk);
		if(!conversionOk)
		{
			fontSize = 11;
		}
		m_proxy->setFont(QFont(fontFamily, fontSize));
	}

	ui->textView->setWordWrap(true);
	ui->textView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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

void LogPage::on_InstanceLaunchTask_changed(std::shared_ptr<LaunchTask> proc)
{
	m_process = proc;
	if(m_process)
	{
		m_model = proc->getLogModel();
		auto lineSetting = MMC->settings()->getSetting("ConsoleMaxLines");
		bool conversionOk = false;
		int maxLines = lineSetting->get().toInt(&conversionOk);
		if(!conversionOk)
		{
			maxLines = lineSetting->defValue().toInt();
			qWarning() << "ConsoleMaxLines has nonsensical value, defaulting to" << maxLines;
		}
		m_model->setMaxLines(maxLines);
		m_model->setStopOnOverflow(MMC->settings()->get("ConsoleOverflowStop").toBool());
		m_model->setOverflowMessage(tr("MultiMC stopped watching the game log because the log length surpassed %1 lines.\n"
			"You may have to fix your mods because the game is still loggging to files and"
			" likely wasting harddrive space at an alarming rate!").arg(maxLines));
		m_proxy->setSourceModel(m_model.get());
	}
	else
	{
		m_proxy->setSourceModel(nullptr);
		m_model.reset();
	}
}

bool LogPage::apply()
{
	return true;
}

bool LogPage::shouldDisplay() const
{
	return m_instance->isRunning() || m_proxy->rowCount() > 0;
}

void LogPage::on_btnPaste_clicked()
{
	if(!m_model)
		return;

	//FIXME: turn this into a proper task and move the upload logic out of GuiUtil!
	m_model->append(MessageLevel::MultiMC, tr("MultiMC: Log upload triggered at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
	auto url = GuiUtil::uploadPaste(m_model->toPlainText(), this);
	if(!url.isEmpty())
	{
		m_model->append(MessageLevel::MultiMC, tr("MultiMC: Log uploaded to: %1").arg(url));
	}
	else
	{
		m_model->append(MessageLevel::Error, tr("MultiMC: Log upload failed!"));
	}
}

void LogPage::on_btnCopy_clicked()
{
	if(!m_model)
		return;
	m_model->append(MessageLevel::MultiMC, QString("Clipboard copy at: %1").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)));
	GuiUtil::setClipboardText(m_model->toPlainText());
}

void LogPage::on_btnClear_clicked()
{
	if(!m_model)
		return;
	m_model->clear();
}

void LogPage::on_btnBottom_clicked()
{
	/*
	ui->textView->verticalScrollBar()->setSliderPosition(ui->textView->verticalScrollBar()->maximum());
	*/
	auto numRows = m_proxy->rowCount(QModelIndex());
	auto lastIndex = m_proxy->index(numRows - 1, 0 , QModelIndex());
	ui->textView->scrollTo(lastIndex, QAbstractItemView::ScrollHint::EnsureVisible);
}

void LogPage::on_trackLogCheckbox_clicked(bool checked)
{
	m_write_active = checked;
}

void LogPage::on_wrapCheckbox_clicked(bool checked)
{
	if(checked)
	{
		ui->textView->setWordWrap(true);
		ui->textView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
	else
	{
		ui->textView->setWordWrap(false);
		ui->textView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	}
}

void LogPage::findImpl(bool reverse)
{
	auto toSearch = ui->searchBar->text();
	if (toSearch.size())
	{
		auto index = ui->textView->currentIndex();
		if(!index.isValid())
		{
			index = m_proxy->index(0,0);
		}
		if(!index.isValid())
		{
			// just give up
			return;
		}
		auto found = m_proxy->find(index, toSearch, reverse);
		if(found.isValid())
			ui->textView->setCurrentIndex(found);
	}
}

void LogPage::on_findButton_clicked()
{
	auto modifiers = QApplication::keyboardModifiers();
	bool reverse = modifiers & Qt::ShiftModifier;
	findImpl(reverse);
}

void LogPage::findNextActivated()
{
	findImpl(false);
}

void LogPage::findPreviousActivated()
{
	findImpl(true);
}

void LogPage::findActivated()
{
	// focus the search bar if it doesn't have focus
	if (!ui->searchBar->hasFocus())
	{
		ui->searchBar->setFocus();
		ui->searchBar->selectAll();
	}
}

void LogPage::setParentContainer(BasePageContainer * container)
{
	m_parentContainer = container;
}

void LogPage::rowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
	auto numRows = m_proxy->rowCount(QModelIndex());
	auto lastIndex = m_proxy->index(numRows - 1, 0 , QModelIndex());
	auto rect = ui->textView->visualRect(lastIndex);
	auto viewPortRect = ui->textView->viewport()->rect();
	m_autoScroll = rect.intersects(viewPortRect);
}

void LogPage::rowsInserted(const QModelIndex& parent, int first, int last)
{
	if(m_autoScroll)
	{
		QMetaObject::invokeMethod(this, "on_btnBottom_clicked", Qt::QueuedConnection);
	}
}
