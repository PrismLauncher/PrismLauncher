#include "OtherLogsPage.h"
#include "ui_OtherLogsPage.h"

#include <QFileDialog>
#include <QMessageBox>

#include "gui/GuiUtil.h"
#include "logic/RecursiveFileSystemWatcher.h"
#include "logic/BaseInstance.h"

OtherLogsPage::OtherLogsPage(BaseInstance *instance, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::OtherLogsPage),
	m_instance(instance),
	m_watcher(new RecursiveFileSystemWatcher(this))
{
	ui->setupUi(this);
	connect(m_watcher, &RecursiveFileSystemWatcher::filesChanged, [this]()
	{
		ui->selectLogBox->clear();
		ui->selectLogBox->addItems(m_watcher->files());
		ui->selectLogBox->addItem(tr("&Other"), true);
		if (m_currentFile.isNull())
		{
			ui->selectLogBox->setCurrentIndex(-1);
		}
		else
		{
			const int index = ui->selectLogBox->findText(m_currentFile);
			ui->selectLogBox->setCurrentIndex(-1);
		}
	});
}

OtherLogsPage::~OtherLogsPage()
{
	delete ui;
}

void OtherLogsPage::opened()
{
	m_watcher->enable();
}
void OtherLogsPage::closed()
{
	m_watcher->disable();
}

void OtherLogsPage::on_selectLogBox_currentIndexChanged(const int index)
{
	QString file;
	if (index != -1)
	{
		if (ui->selectLogBox->itemData(index).isValid())
		{
			file = QFileDialog::getOpenFileName(this, tr("Open log file"), m_instance->minecraftRoot(), tr("*.log;;*.txt;;*"));
		}
		else
		{
			file = ui->selectLogBox->itemText(index);
		}
	}

	if (file.isEmpty() || !QFile::exists(file))
	{
		m_currentFile = QString();
		setControlsEnabled(false);
	}
	else
	{
		m_currentFile = file;
		on_btnReload_clicked();
		setControlsEnabled(true);
	}
}

void OtherLogsPage::on_btnReload_clicked()
{
	QFile file(m_currentFile);
	if (!file.open(QFile::ReadOnly))
	{
		setControlsEnabled(false);
		ui->btnReload->setEnabled(true); // allow reload
		m_currentFile = QString();
		QMessageBox::critical(this, tr("Error"), tr("Unable to open %1 for reading: %2").arg(m_currentFile, file.errorString()));
	}
	else
	{
		ui->text->setPlainText(QString::fromUtf8(file.readAll()));
	}
}

void OtherLogsPage::on_btnPaste_clicked()
{
	GuiUtil::uploadPaste(ui->text->toPlainText(), this);
}
void OtherLogsPage::on_btnCopy_clicked()
{
	GuiUtil::setClipboardText(ui->text->toPlainText());
}
void OtherLogsPage::on_btnDelete_clicked()
{
	if (QMessageBox::question(this, tr("Delete"), tr("Do you really want to delete %1?").arg(m_currentFile), QMessageBox::Yes, QMessageBox::No)
			== QMessageBox::No)
	{
		return;
	}
	QFile file(m_currentFile);
	if (!file.remove())
	{
		QMessageBox::critical(this, tr("Error"), tr("Unable to delete %1: %2").arg(m_currentFile, file.errorString()));
	}
}

void OtherLogsPage::setControlsEnabled(const bool enabled)
{
	ui->btnReload->setEnabled(enabled);
	ui->btnDelete->setEnabled(enabled);
	ui->btnCopy->setEnabled(enabled);
	ui->btnPaste->setEnabled(enabled);
	ui->text->setEnabled(enabled);
}
