#pragma once

#include <QAbstractListModel>
#include "logic/BaseInstance.h"
#include "logic/tasks/Task.h"

class ScreenShot
{
public:
	QDateTime timestamp;
	QString file;
	QString url;
	QString imgurIndex;
};

class ScreenshotList : public QAbstractListModel
{
	Q_OBJECT
public:
	ScreenshotList(BaseInstance *instance, QObject *parent = 0);

	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	int rowCount(const QModelIndex &parent) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;

	Task *load();

	void loadShots(QList<ScreenShot *> shots)
	{
		m_screenshots = shots;
	}

	QList<ScreenShot *> screenshots() const
	{
		return m_screenshots;
	}

	BaseInstance *instance() const
	{
		return m_instance;
	}

signals:

public
slots:

private:
	QList<ScreenShot *> m_screenshots;
	BaseInstance *m_instance;
};

class ScreenshotLoadTask : public Task
{
	Q_OBJECT

public:
	explicit ScreenshotLoadTask(ScreenshotList *list);
	~ScreenshotLoadTask();

	QList<ScreenShot *> screenShots() const
	{
		return m_results;
	}

protected:
	virtual void executeTask();

private:
	ScreenshotList *m_list;
	QList<ScreenShot *> m_results;
};
