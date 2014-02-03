#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include <QStandardItem>
#include <QDebug>

#include "GroupView.h"

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
		item->setData(1000, GroupViewRoles::ProgressMaximumRole);
		m_items.append(item);
		return item;
	}

public
slots:
	void timeout()
	{
		QList<QStandardItem *> toRemove;
		for (auto item : m_items)
		{
			int maximum = item->data(GroupViewRoles::ProgressMaximumRole).toInt();
			int value = item->data(GroupViewRoles::ProgressValueRole).toInt();
			int newvalue = std::min(value + 3, maximum);
			item->setData(newvalue, GroupViewRoles::ProgressValueRole);

			if(newvalue >= maximum)
			{
				toRemove.append(item);
			}
		}
		for(auto remove : toRemove)
		{
			m_items.removeAll(remove);
		}
	}

private:
	QList<QStandardItem *> m_items;
};
