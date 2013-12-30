#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QStandardItem>
#include <QDebug>

#include "CategorizedView.h"

class Progresser : public QObject
{
	Q_OBJECT
public:
	explicit Progresser(QObject *parent = 0) : QObject(parent)
	{
		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
		timer->start(50);
	}

	QStandardItem *addTrackedIndex(QStandardItem *item)
	{
		item->setData(1000, CategorizedViewRoles::ProgressMaximumRole);
		m_items.append(item);
		return item;
	}

public slots:
	void timeout()
	{
		foreach (QStandardItem *item, m_items)
		{
			int value = item->data(CategorizedViewRoles::ProgressValueRole).toInt();
			value += qrand() % 3;
			if (value >= item->data(CategorizedViewRoles::ProgressMaximumRole).toInt())
			{
				item->setData(item->data(CategorizedViewRoles::ProgressMaximumRole).toInt(),
							  CategorizedViewRoles::ProgressValueRole);
			}
			else
			{
				item->setData(value, CategorizedViewRoles::ProgressValueRole);
			}
		}
	}

private:
	QList<QStandardItem *> m_items;
};

#endif // MAIN_H
