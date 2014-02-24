#include "ScreenshotList.h"

#include <QDir>
#include <QIcon>

ScreenshotList::ScreenshotList(BaseInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
}

int ScreenshotList::rowCount(const QModelIndex &) const
{
	return m_screenshots.size();
}

QVariant ScreenshotList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	switch (role)
	{
	case Qt::DecorationRole:
		return QIcon(m_screenshots.at(index.row())->file);
	case Qt::DisplayRole:
		return m_screenshots.at(index.row())->timestamp.toString("yyyy-MM-dd HH:mm:ss");
	case Qt::ToolTipRole:
		return m_screenshots.at(index.row())->timestamp.toString("yyyy-MM-dd HH:mm:ss");
	case Qt::TextAlignmentRole:
		return (int)(Qt::AlignHCenter | Qt::AlignVCenter);
	default:
		return QVariant();
	}
}

QVariant ScreenshotList::headerData(int section, Qt::Orientation orientation, int role) const
{
	return QVariant();
}

Qt::ItemFlags ScreenshotList::flags(const QModelIndex &index) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

Task *ScreenshotList::load()
{
	return new ScreenshotLoadTask(this);
}

ScreenshotLoadTask::ScreenshotLoadTask(ScreenshotList *list) : m_list(list)
{
}

ScreenshotLoadTask::~ScreenshotLoadTask()
{
}

void ScreenshotLoadTask::executeTask()
{
	auto dir = QDir(m_list->instance()->minecraftRoot());
	if (!dir.cd("screenshots"))
	{
		emitFailed("Selected instance does not have any screenshots!");
		return;
	}
	dir.setNameFilters(QStringList() << "*.png");
	this->m_results = QList<ScreenShot *>();
	for (auto file : dir.entryList())
	{
		ScreenShot *shot = new ScreenShot();
		shot->timestamp = QDateTime::fromString(file, "yyyy-MM-dd_HH.mm.ss.png");
		shot->file = dir.absoluteFilePath(file);
		m_results.append(shot);
	}
	m_list->loadShots(m_results);
	emitSucceeded();
}
