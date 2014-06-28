#include "ScreenshotsPage.h"
#include "ui_ScreenshotsPage.h"

#include <QModelIndex>
#include <QMutableListIterator>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QtGui/qevent.h>

#include <pathutils.h>

#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "logic/screenshots/ScreenshotList.h"
#include "logic/net/NetJob.h"
#include "logic/screenshots/ImgurUpload.h"
#include "logic/screenshots/ImgurAlbumCreation.h"
#include "logic/tasks/SequentialTask.h"

class ThumbnailProvider : public QFileIconProvider
{
public:
	virtual ~ThumbnailProvider() {};
	virtual QIcon icon(const QFileInfo &info) const
	{
		QImage image(info.absoluteFilePath());
		if (image.isNull())
		{
			return QFileIconProvider::icon(info);
		}
		QImage thumbnail = image.scaledToWidth(256, Qt::SmoothTransformation);
		return QIcon(QPixmap::fromImage(thumbnail));
	}
};

class FilterModel : public QIdentityProxyModel
{
	virtual QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const
	{
		auto model = sourceModel();
		if(!model)
			return QVariant();
		QVariant result = sourceModel()->data(mapToSource(proxyIndex), role);
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			return result.toString().remove(QRegExp("\.png$"));
		}
		return result;
	}
	virtual bool setData(const QModelIndex &index, const QVariant &value,
						int role = Qt::EditRole)
	{
		auto model = sourceModel();
		if(!model)
			return false;
		if(role != Qt::EditRole)
			return false;
		// FIXME: this is a workaround for a bug in QFileSystemModel, where it doesn't
		// sort after renames
		{
			((QFileSystemModel *)model)->setNameFilterDisables(true);
			((QFileSystemModel *)model)->setNameFilterDisables(false);
		}
		return model->setData(mapToSource(index), value.toString() + ".png", role);
	}
};

class CenteredEditingDelegate : public QStyledItemDelegate
{
public:
	explicit CenteredEditingDelegate(QObject *parent = 0)
	: QStyledItemDelegate(parent)
	{
	}
	virtual ~CenteredEditingDelegate() {}
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
								  const QModelIndex &index) const
	{
		auto widget = QStyledItemDelegate::createEditor(parent, option, index);
		auto foo = dynamic_cast<QLineEdit *> (widget);
		if(foo)
		{
			foo->setAlignment(Qt::AlignHCenter);
			foo->setFrame(true);
			foo->setMaximumWidth(192);
		}
		return widget;
	}
};

QString ScreenshotsPage::displayName()
{
	return tr("Screenshots");
}

QIcon ScreenshotsPage::icon()
{
	return QIcon::fromTheme("screenshots");
}

QString ScreenshotsPage::id()
{
	return "screenshots";
}

ScreenshotsPage::ScreenshotsPage(BaseInstance *instance, QWidget *parent)
	: QWidget(parent), ui(new Ui::ScreenshotsPage)
{
	m_model.reset(new QFileSystemModel());
	m_filterModel.reset(new FilterModel());
	m_filterModel->setSourceModel(m_model.get());
	m_model->setFilter(QDir::Files | QDir::Writable | QDir::Readable);
	m_model->setIconProvider(new ThumbnailProvider);
	m_model->setReadOnly(false);
	m_folder = PathCombine(instance->minecraftRoot(), "screenshots");
	m_valid = ensureFolderPathExists(m_folder);

	ui->setupUi(this);
	ui->listView->setModel(m_filterModel.get());
	ui->listView->setIconSize(QSize(128, 128));
	ui->listView->setGridSize(QSize(192, 128));
	ui->listView->setSpacing(9);
	// ui->listView->setUniformItemSizes(true);
	ui->listView->setLayoutMode(QListView::Batched);
	ui->listView->setViewMode(QListView::IconMode);
	ui->listView->setResizeMode(QListView::Adjust);
	ui->listView->installEventFilter(this);
	ui->listView->setEditTriggers(0);
	ui->listView->setItemDelegate(new CenteredEditingDelegate(this));
	connect(ui->listView, SIGNAL(activated(QModelIndex)), SLOT(onItemActivated(QModelIndex)));
}

bool ScreenshotsPage::eventFilter(QObject *obj, QEvent *evt)
{
	if (obj != ui->listView)
		return QWidget::eventFilter(obj, evt);
	if (evt->type() != QEvent::KeyPress)
	{
		return QWidget::eventFilter(obj, evt);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(evt);
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_deleteBtn_clicked();
		return true;
	case Qt::Key_F2:
		on_renameBtn_clicked();
		return true;
	default:
		break;
	}
	return QWidget::eventFilter(obj, evt);
}

ScreenshotsPage::~ScreenshotsPage()
{
	delete ui;
}

void ScreenshotsPage::onItemActivated(QModelIndex index)
{
	if (!index.isValid())
		return;
	auto info = m_model->fileInfo(index);
	QString fileName = info.absoluteFilePath();
	openFileInDefaultProgram(info.absoluteFilePath());
}

void ScreenshotsPage::on_viewFolderBtn_clicked()
{
	openDirInDefaultProgram(m_folder, true);
}

void ScreenshotsPage::on_uploadBtn_clicked()
{
	auto selection = ui->listView->selectionModel()->selectedIndexes();
	if (selection.isEmpty())
		return;

	QList<ScreenshotPtr> uploaded;
	auto job = std::make_shared<NetJob>("Screenshot Upload");
	for (auto item : selection)
	{
		auto info = m_model->fileInfo(item);
		auto screenshot = std::make_shared<ScreenShot>(info);
		uploaded.push_back(screenshot);
		job->addNetAction(ImgurUpload::make(screenshot));
	}
	SequentialTask task;
	auto albumTask = std::make_shared<NetJob>("Imgur Album Creation");
	auto imgurAlbum = ImgurAlbumCreation::make(uploaded);
	albumTask->addNetAction(imgurAlbum);
	task.addTask(job);
	task.addTask(albumTask);
	ProgressDialog prog(this);
	if (prog.exec(&task) != QDialog::Accepted)
	{
		CustomMessageBox::selectable(this, tr("Failed to upload screenshots!"),
									 tr("Unknown error"), QMessageBox::Warning)->exec();
	}
	else
	{
		CustomMessageBox::selectable(
			this, tr("Upload finished"),
			tr("<a href=\"https://imgur.com/a/%1\">Visit album</a><br/>Delete hash: %2 (save "
			   "this if you want to be able to edit/delete the album)")
				.arg(imgurAlbum->id(), imgurAlbum->deleteHash()),
			QMessageBox::Information)->exec();
	}
}

void ScreenshotsPage::on_deleteBtn_clicked()
{
	auto mbox = CustomMessageBox::selectable(
		this, tr("Are you sure?"), tr("This will delete all selected screenshots."),
		QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No);
	std::unique_ptr<QMessageBox> box(mbox);

	if(box->exec() != QMessageBox::Yes)
		return;

	auto selected = ui->listView->selectionModel()->selectedIndexes();
	for(auto item : selected)
	{
		m_model->remove(item);
	}
}

void ScreenshotsPage::on_renameBtn_clicked()
{
	auto selection = ui->listView->selectionModel()->selectedIndexes();
	if (selection.isEmpty())
		return;
	ui->listView->edit(selection[0]);
	// TODO: mass renaming
}

void ScreenshotsPage::opened()
{
	if (m_valid)
	{
		QString path = QDir(m_folder).absolutePath();
		m_model->setRootPath(path);
		ui->listView->setRootIndex(m_filterModel->mapFromSource(m_model->index(path)));
	}
}
