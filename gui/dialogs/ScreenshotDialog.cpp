#include "ScreenshotDialog.h"
#include "ui_ScreenshotDialog.h"

#include <QModelIndex>
#include <QDebug>

#include "ProgressDialog.h"
#include "CustomMessageBox.h"
#include "logic/net/NetJob.h"
#include "logic/net/ScreenshotUploader.h"

ScreenshotDialog::ScreenshotDialog(ScreenshotList *list, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ScreenshotDialog),
	m_list(list)
{
	ui->setupUi(this);
	ui->listView->setModel(m_list);
}

ScreenshotDialog::~ScreenshotDialog()
{
	delete ui;
}

QList<ScreenShot *> ScreenshotDialog::uploaded() const
{
	return m_uploaded;
}

QList<ScreenShot*> ScreenshotDialog::selected() const
{
	QList<ScreenShot*> list;
	QList<ScreenShot*> first = m_list->screenshots();
	for (QModelIndex index : ui->listView->selectionModel()->selectedRows())
	{
		list.append(first.at(index.row()));
	}
	return list;
}

void ScreenshotDialog::on_buttonBox_accepted()
{
	QList<ScreenShot *> screenshots = selected();
	if (screenshots.isEmpty())
	{
		done(NothingDone);
		return;
	}
	NetJob *job = new NetJob("Screenshot Upload");
	for (ScreenShot *shot : screenshots)
	{
		qDebug() << shot->file;
		job->addNetAction(ScreenShotUpload::make(shot));
	}
	ProgressDialog prog(this);
	prog.exec(job);
	connect(job, &NetJob::failed, [this]
	{
		CustomMessageBox::selectable(this, tr("Failed to upload screenshots!"),
									 tr("Unknown error"), QMessageBox::Warning)->exec();
		reject();
	});
	m_uploaded = screenshots;
	connect(job, &NetJob::succeeded, this, &ScreenshotDialog::accept);
}
