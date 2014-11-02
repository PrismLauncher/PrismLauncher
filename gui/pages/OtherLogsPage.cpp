/* Copyright 2013-2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "OtherLogsPage.h"
#include "ui_OtherLogsPage.h"

#include <QFileDialog>
#include <QMessageBox>

#include "gui/GuiUtil.h"
#include "logic/RecursiveFileSystemWatcher.h"
#include "logic/BaseInstance.h"

OtherLogsPage::OtherLogsPage(BaseInstance *instance, QWidget *parent)
	: QWidget(parent), ui(new Ui::OtherLogsPage), m_instance(instance),
	  m_watcher(new RecursiveFileSystemWatcher(this))
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	m_watcher->setFileExpression("(.*\\.log(\\.[0-9]*)?$)|(crash-.*\\.txt)");
	m_watcher->setRootDir(QDir::current().absoluteFilePath(m_instance->minecraftRoot()));

	connect(m_watcher, &RecursiveFileSystemWatcher::filesChanged, this,
			&OtherLogsPage::populateSelectLogBox);
	populateSelectLogBox();
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

void OtherLogsPage::populateSelectLogBox()
{
	ui->selectLogBox->clear();
	ui->selectLogBox->addItems(m_watcher->files());
	if (m_currentFile.isNull())
	{
		ui->selectLogBox->setCurrentIndex(-1);
	}
	else
	{
		const int index = ui->selectLogBox->findText(m_currentFile);
		if (index != -1)
			ui->selectLogBox->setCurrentIndex(index);
	}
}

void OtherLogsPage::on_selectLogBox_currentIndexChanged(const int index)
{
	QString file;
	if (index != -1)
	{
		file = ui->selectLogBox->itemText(index);
	}

	if (file.isEmpty() || !QFile::exists(m_instance->minecraftRoot() + "/" + file))
	{
		m_currentFile = QString();
		ui->text->clear();
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
	QFile file(m_instance->minecraftRoot() + "/" + m_currentFile);
	if (!file.open(QFile::ReadOnly))
	{
		setControlsEnabled(false);
		ui->btnReload->setEnabled(true); // allow reload
		m_currentFile = QString();
		QMessageBox::critical(this, tr("Error"), tr("Unable to open %1 for reading: %2")
													 .arg(m_currentFile, file.errorString()));
	}
	else
	{
		if (file.size() < 10000000ll)
		{
			ui->text->setPlainText(QString::fromUtf8(file.readAll()));
		}
		else
		{
			ui->text->setPlainText(
				tr("The file (%1) is too big. You may want to open it in a viewer optimized "
				   "for large files.").arg(file.fileName()));
		}
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
	if (QMessageBox::question(this, tr("Delete"),
							  tr("Do you really want to delete %1?").arg(m_currentFile),
							  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		return;
	}
	QFile file(m_instance->minecraftRoot() + "/" + m_currentFile);
	if (!file.remove())
	{
		QMessageBox::critical(this, tr("Error"), tr("Unable to delete %1: %2")
													 .arg(m_currentFile, file.errorString()));
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
