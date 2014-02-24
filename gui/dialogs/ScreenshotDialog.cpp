#include "ScreenshotDialog.h"
#include "ui_ScreenshotDialog.h"

#include <QModelIndex>
#include <QMutableListIterator>

#include "ProgressDialog.h"
#include "CustomMessageBox.h"
#include "logic/net/NetJob.h"
#include "logic/net/ImgurUpload.h"
#include "logic/net/ImgurAlbumCreation.h"
#include "logic/tasks/SequentialTask.h"

ScreenshotDialog::ScreenshotDialog(ScreenshotList *list, QWidget *parent)
	: QDialog(parent), ui(new Ui::ScreenshotDialog), m_list(list)
{
	ui->setupUi(this);
	ui->listView->setModel(m_list);
}

ScreenshotDialog::~ScreenshotDialog()
{
	delete ui;
}

QString ScreenshotDialog::message() const
{
	return tr("<a href=\"https://imgur.com/a/%1\">Visit album</a><br/>Delete hash: %2 (save "
			  "this if you want to be able to edit/delete the album)")
		.arg(m_imgurAlbum->id(), m_imgurAlbum->deleteHash());
}

QList<ScreenShot *> ScreenshotDialog::selected() const
{
	QList<ScreenShot *> list;
	QList<ScreenShot *> first = m_list->screenshots();
	for (QModelIndex index : ui->listView->selectionModel()->selectedRows())
	{
		list.append(first.at(index.row()));
	}
	return list;
}

void ScreenshotDialog::on_uploadBtn_clicked()
{
	m_uploaded = selected();
	if (m_uploaded.isEmpty())
	{
		done(NothingDone);
		return;
	}
	SequentialTask *task = new SequentialTask(this);
	NetJob *job = new NetJob("Screenshot Upload");
	for (ScreenShot *shot : m_uploaded)
	{
		job->addNetAction(ImgurUpload::make(shot));
	}
	NetJob *albumTask = new NetJob("Imgur Album Creation");
	albumTask->addNetAction(m_imgurAlbum = ImgurAlbumCreation::make(m_uploaded));
	task->addTask(std::shared_ptr<NetJob>(job));
	task->addTask(std::shared_ptr<NetJob>(albumTask));
	ProgressDialog prog(this);
	if (prog.exec(task) == QDialog::Accepted)
	{
		accept();
	}
	else
	{
		CustomMessageBox::selectable(this, tr("Failed to upload screenshots!"),
									 tr("Unknown error"), QMessageBox::Warning)->exec();
		reject();
	}
}

void ScreenshotDialog::on_deleteBtn_clicked()
{
	m_list->deleteSelected(this);
}
