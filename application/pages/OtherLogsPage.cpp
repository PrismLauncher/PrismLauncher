/* Copyright 2013-2017 MultiMC Contributors
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

#include <QMessageBox>

#include "GuiUtil.h"
#include "RecursiveFileSystemWatcher.h"
#include <GZip.h>
#include <FileSystem.h>
#include <QShortcut>

OtherLogsPage::OtherLogsPage(QString path, IPathMatcher::Ptr fileFilter, QWidget *parent)
	: QWidget(parent), ui(new Ui::OtherLogsPage), m_path(path), m_fileFilter(fileFilter),
	  m_watcher(new RecursiveFileSystemWatcher(this))
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	m_watcher->setMatcher(fileFilter);
	m_watcher->setRootDir(QDir::current().absoluteFilePath(m_path));

	connect(m_watcher, &RecursiveFileSystemWatcher::filesChanged, this, &OtherLogsPage::populateSelectLogBox);
	populateSelectLogBox();

	auto findShortcut = new QShortcut(QKeySequence(QKeySequence::Find), this);
	connect(findShortcut, &QShortcut::activated, this, &OtherLogsPage::findActivated);

	auto findNextShortcut = new QShortcut(QKeySequence(QKeySequence::FindNext), this);
	connect(findNextShortcut, &QShortcut::activated, this, &OtherLogsPage::findNextActivated);

	auto findPreviousShortcut = new QShortcut(QKeySequence(QKeySequence::FindPrevious), this);
	connect(findPreviousShortcut, &QShortcut::activated, this, &OtherLogsPage::findPreviousActivated);

	connect(ui->searchBar, &QLineEdit::returnPressed, this, &OtherLogsPage::on_findButton_clicked);
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
	if (m_currentFile.isEmpty())
	{
		setControlsEnabled(false);
		ui->selectLogBox->setCurrentIndex(-1);
	}
	else
	{
		const int index = ui->selectLogBox->findText(m_currentFile);
		if (index != -1)
		{
			ui->selectLogBox->setCurrentIndex(index);
			setControlsEnabled(true);
		}
		else
		{
			setControlsEnabled(false);
		}
	}
}

void OtherLogsPage::on_selectLogBox_currentIndexChanged(const int index)
{
	QString file;
	if (index != -1)
	{
		file = ui->selectLogBox->itemText(index);
	}

	if (file.isEmpty() || !QFile::exists(FS::PathCombine(m_path, file)))
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
	if(m_currentFile.isEmpty())
	{
		setControlsEnabled(false);
		return;
	}
	QFile file(FS::PathCombine(m_path, m_currentFile));
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
		auto showTooBig = [&]()
		{
			ui->text->setPlainText(
				tr("The file (%1) is too big. You may want to open it in a viewer optimized "
				   "for large files.").arg(file.fileName()));
		};
		if(file.size() > (1024ll * 1024ll * 12ll))
		{
			showTooBig();
			return;
		}
		QString content;
		if(file.fileName().endsWith(".gz"))
		{
			QByteArray temp;
			if(!GZip::unzip(file.readAll(), temp))
			{
				ui->text->setPlainText(
					tr("The file (%1) is not readable.").arg(file.fileName()));
				return;
			}
			content = QString::fromUtf8(temp);
		}
		else
		{
			content = QString::fromUtf8(file.readAll());
		}
		if (content.size() >= 50000000ll)
		{
			showTooBig();
			return;
		}
		ui->text->setPlainText(content);
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
	if(m_currentFile.isEmpty())
	{
		setControlsEnabled(false);
		return;
	}
	if (QMessageBox::question(this, tr("Delete"),
							  tr("Do you really want to delete %1?").arg(m_currentFile),
							  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		return;
	}
	QFile file(FS::PathCombine(m_path, m_currentFile));
	if (!file.remove())
	{
		QMessageBox::critical(this, tr("Error"), tr("Unable to delete %1: %2")
													 .arg(m_currentFile, file.errorString()));
	}
}



void OtherLogsPage::on_btnClean_clicked()
{
	auto toDelete = m_watcher->files();
	if(toDelete.isEmpty())
	{
		return;
	}
	QMessageBox *messageBox = new QMessageBox(this);
	messageBox->setWindowTitle(tr("Clean up"));
	if(toDelete.size() > 5)
	{
		messageBox->setText(tr("Do you really want to delete all log files?"));
		messageBox->setDetailedText(toDelete.join('\n'));
	}
	else
	{
		messageBox->setText(tr("Do you really want to delete these files?\n%1").arg(toDelete.join('\n')));
	}
	messageBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	messageBox->setDefaultButton(QMessageBox::Ok);
	messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
	messageBox->setIcon(QMessageBox::Question);
	messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);

	if (messageBox->exec() != QMessageBox::Ok)
	{
		return;
	}
	QStringList failed;
	for(auto item: toDelete)
	{
		QFile file(FS::PathCombine(m_path, item));
		if (!file.remove())
		{
			failed.push_back(item);
		}
	}
	if(!failed.empty())
	{
		QMessageBox *messageBox = new QMessageBox(this);
		messageBox->setWindowTitle(tr("Error"));
		if(failed.size() > 5)
		{
			messageBox->setText(tr("Couldn't delete some files!"));
			messageBox->setDetailedText(failed.join('\n'));
		}
		else
		{
			messageBox->setText(tr("Couldn't delete some files:\n%1").arg(failed.join('\n')));
		}
		messageBox->setStandardButtons(QMessageBox::Ok);
		messageBox->setDefaultButton(QMessageBox::Ok);
		messageBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
		messageBox->setIcon(QMessageBox::Critical);
		messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
		messageBox->exec();
	}
}


void OtherLogsPage::setControlsEnabled(const bool enabled)
{
	ui->btnReload->setEnabled(enabled);
	ui->btnDelete->setEnabled(enabled);
	ui->btnCopy->setEnabled(enabled);
	ui->btnPaste->setEnabled(enabled);
	ui->text->setEnabled(enabled);
	ui->btnClean->setEnabled(enabled);
}

// FIXME: HACK, use LogView instead?
static void findNext(QPlainTextEdit * _this, const QString& what, bool reverse)
{
	_this->find(what, reverse ? QTextDocument::FindFlag::FindBackward : QTextDocument::FindFlag(0));
}

void OtherLogsPage::on_findButton_clicked()
{
	auto modifiers = QApplication::keyboardModifiers();
	bool reverse = modifiers & Qt::ShiftModifier;
	findNext(ui->text, ui->searchBar->text(), reverse);
}

void OtherLogsPage::findNextActivated()
{
	findNext(ui->text, ui->searchBar->text(), false);
}

void OtherLogsPage::findPreviousActivated()
{
	findNext(ui->text, ui->searchBar->text(), true);
}

void OtherLogsPage::findActivated()
{
	// focus the search bar if it doesn't have focus
	if (!ui->searchBar->hasFocus())
	{
		ui->searchBar->setFocus();
		ui->searchBar->selectAll();
	}
}
