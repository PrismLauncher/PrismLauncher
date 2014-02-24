#include "ScreenshotDialog.h"
#include "ui_ScreenshotDialog.h"
#include "QModelIndex"

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

QList<ScreenShot*> ScreenshotDialog::selected()
{
	QList<ScreenShot*> list;
	QList<ScreenShot*> first = m_list->screenshots();
	for (QModelIndex index : ui->listView->selectionModel()->selectedRows())
	{
		list.append(first.at(index.row()));
	}
	return list;
}
