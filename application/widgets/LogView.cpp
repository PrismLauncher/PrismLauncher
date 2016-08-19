#include "LogView.h"
#include <QTextBlock>
#include <QScrollBar>

LogView::LogView(QWidget* parent) : QPlainTextEdit(parent)
{
	setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	m_defaultFormat = new QTextCharFormat(currentCharFormat());
}

LogView::~LogView()
{
	delete m_defaultFormat;
}

void LogView::setWordWrap(bool wrapping)
{
	if(wrapping)
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setLineWrapMode(QPlainTextEdit::WidgetWidth);
	}
	else
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		setLineWrapMode(QPlainTextEdit::NoWrap);
	}
}

void LogView::setModel(QAbstractItemModel* model)
{
	if(m_model)
	{
		disconnect(m_model, &QAbstractItemModel::modelReset, this, &LogView::repopulate);
		disconnect(m_model, &QAbstractItemModel::rowsInserted, this, &LogView::rowsInserted);
		disconnect(m_model, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogView::rowsAboutToBeInserted);
		disconnect(m_model, &QAbstractItemModel::rowsRemoved, this, &LogView::rowsRemoved);
	}
	m_model = model;
	if(m_model)
	{
		connect(m_model, &QAbstractItemModel::modelReset, this, &LogView::repopulate);
		connect(m_model, &QAbstractItemModel::rowsInserted, this, &LogView::rowsInserted);
		connect(m_model, &QAbstractItemModel::rowsAboutToBeInserted, this, &LogView::rowsAboutToBeInserted);
		connect(m_model, &QAbstractItemModel::rowsRemoved, this, &LogView::rowsRemoved);
		connect(m_model, &QAbstractItemModel::destroyed, this, &LogView::modelDestroyed);
	}
	repopulate();
}

QAbstractItemModel * LogView::model() const
{
	return m_model;
}

void LogView::modelDestroyed(QObject* model)
{
	if(m_model == model)
	{
		setModel(nullptr);
	}
}

void LogView::repopulate()
{
	auto doc = document();
	doc->clear();
	if(!m_model)
	{
		return;
	}
	rowsInserted(QModelIndex(), 0, m_model->rowCount() - 1);
}

void LogView::rowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
	Q_UNUSED(parent)
	Q_UNUSED(first)
	Q_UNUSED(last)
	QScrollBar *bar = verticalScrollBar();
	int max_bar = bar->maximum();
	int val_bar = bar->value();
	if (m_scroll)
	{
		m_scroll = (max_bar - val_bar) <= 1;
	}
	else
	{
		m_scroll = val_bar == max_bar;
	}
}

void LogView::rowsInserted(const QModelIndex& parent, int first, int last)
{
	for(int i = first; i <= last; i++)
	{
		auto idx = m_model->index(i, 0, parent);
		auto text = m_model->data(idx, Qt::DisplayRole).toString();
		QTextCharFormat format(*m_defaultFormat);
		auto font = m_model->data(idx, Qt::FontRole);
		if(font.isValid())
		{
			format.setFont(font.value<QFont>());
		}
		auto fg = m_model->data(idx, Qt::TextColorRole);
		if(fg.isValid())
		{
			format.setForeground(fg.value<QColor>());
		}
		auto bg = m_model->data(idx, Qt::BackgroundRole);
		if(bg.isValid())
		{
			format.setBackground(bg.value<QColor>());
		}
		auto workCursor = textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(text, format);
		workCursor.insertBlock();
	}
	if(m_scroll && !m_scrolling)
	{
		m_scrolling = true;
		QMetaObject::invokeMethod( this, "scrollToBottom", Qt::QueuedConnection);
	}
}

void LogView::rowsRemoved(const QModelIndex& parent, int first, int last)
{
	// TODO: some day... maybe
	Q_UNUSED(parent)
	Q_UNUSED(first)
	Q_UNUSED(last)
}

void LogView::scrollToBottom()
{
	m_scrolling = false;
	verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());
}

void LogView::findNext(const QString& what, bool reverse)
{
	find(what, reverse ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0));
}
