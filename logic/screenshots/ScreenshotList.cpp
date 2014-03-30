#include "ScreenshotList.h"
#include "gui/dialogs/ScreenshotDialog.h"

#include <QDir>
#include <QIcon>
#include <QList>
#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/CustomMessageBox.h"

ScreenshotList::ScreenshotList(InstancePtr instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
}

int ScreenshotList::rowCount(const QModelIndex &) const
{
	return m_screenshots.size();
}

QVariant ScreenshotList::data(const QModelIndex &index, int role) const
{
	if (index.row() >= m_screenshots.size() || index.row() < 0)
		return QVariant();

	switch (role)
	{
	case Qt::DecorationRole:
		return m_screenshots.at(index.row())->getImage();
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
	this->m_results.clear();
	for (auto file : dir.entryList())
	{
		ScreenShot *shot = new ScreenShot();
		shot->timestamp = QDateTime::fromString(file, "yyyy-MM-dd_HH.mm.ss.png");
		shot->file = dir.absoluteFilePath(file);
		m_results.append(ScreenshotPtr(shot));
	}
	m_list->loadShots(m_results);
	emitSucceeded();
}

void ScreenshotList::deleteSelected(ScreenshotDialog *dialog)
{
	auto screens = dialog->selected();
	if (screens.isEmpty())
	{
		return;
	}
	beginResetModel();
	QList<std::shared_ptr<ScreenShot>>::const_iterator it;
	for (it = screens.cbegin(); it != screens.cend(); it++)
	{
		auto shot = *it;
		if (!QFile(shot->file).remove())
		{
			CustomMessageBox::selectable(dialog, tr("Error!"),
										 tr("Failed to delete screenshots!"),
										 QMessageBox::Warning)->exec();
			break;
		}
	}
	ProgressDialog refresh(dialog);
	Task *t = load();
	if (refresh.exec(t) != QDialog::Accepted)
	{
		CustomMessageBox::selectable(dialog, tr("Error!"),
									 tr("Unable to refresh list: %1").arg(t->failReason()),
									 QMessageBox::Warning)->exec();
	}
	endResetModel();
}
