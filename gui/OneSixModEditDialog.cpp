/* Copyright 2013 MultiMC Contributors
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
#include "MultiMC.h"
#include "OneSixModEditDialog.h"
#include "ModEditDialogCommon.h"
#include "ui_OneSixModEditDialog.h"
#include "logic/ModList.h"
#include "logic/OneSixVersion.h"
#include "logic/EnabledItemFilter.h"
#include "logic/lists/ForgeVersionList.h"
#include <logic/ForgeInstaller.h>
#include "gui/versionselectdialog.h"
#include "ProgressDialog.h"

#include <pathutils.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

OneSixModEditDialog::OneSixModEditDialog(OneSixInstance *inst, QWidget *parent)
	: m_inst(inst), QDialog(parent), ui(new Ui::OneSixModEditDialog)
{
	ui->setupUi(this);
	// libraries!

	m_version = m_inst->getFullVersion();
	if (m_version)
	{
		main_model = new EnabledItemFilter(this);
		main_model->setActive(true);
		main_model->setSourceModel(m_version.data());
		ui->libraryTreeView->setModel(main_model);
		ui->libraryTreeView->installEventFilter(this);
		ui->mainClassEdit->setText(m_version->mainClass);
		updateVersionControls();
	}
	else
	{
		disableVersionControls();
	}
	// Loader mods
	{
		ensureFolderPathExists(m_inst->loaderModsDir());
		m_mods = m_inst->loaderModList();
		ui->loaderModTreeView->setModel(m_mods.data());
		ui->loaderModTreeView->installEventFilter(this);
		m_mods->startWatching();
	}
	// resource packs
	{
		ensureFolderPathExists(m_inst->resourcePacksDir());
		m_resourcepacks = m_inst->resourcePackList();
		ui->resPackTreeView->setModel(m_resourcepacks.data());
		ui->resPackTreeView->installEventFilter(this);
		m_resourcepacks->startWatching();
	}
}

OneSixModEditDialog::~OneSixModEditDialog()
{
	m_mods->stopWatching();
	m_resourcepacks->stopWatching();
	delete ui;
}

void OneSixModEditDialog::updateVersionControls()
{
	bool customVersion = m_inst->versionIsCustom();
	ui->customizeBtn->setEnabled(!customVersion);
	ui->revertBtn->setEnabled(customVersion);
	ui->forgeBtn->setEnabled(true);
}

void OneSixModEditDialog::disableVersionControls()
{
	ui->customizeBtn->setEnabled(false);
	ui->revertBtn->setEnabled(false);
	ui->forgeBtn->setEnabled(false);
}

void OneSixModEditDialog::on_customizeBtn_clicked()
{
	if (m_inst->customizeVersion())
	{
		m_version = m_inst->getFullVersion();
		main_model->setSourceModel(m_version.data());
		updateVersionControls();
	}
}

void OneSixModEditDialog::on_revertBtn_clicked()
{
	auto reply = QMessageBox::question(
		this, tr("Revert?"), tr("Do you want to revert the "
								"version of this instance to its original configuration?"),
		QMessageBox::Yes | QMessageBox::No);
	if (reply == QMessageBox::Yes)
	{
		if (m_inst->revertCustomVersion())
		{
			m_version = m_inst->getFullVersion();
			main_model->setSourceModel(m_version.data());
			updateVersionControls();
		}
	}
}

void OneSixModEditDialog::on_forgeBtn_clicked()
{
	VersionSelectDialog vselect(MMC->forgelist().data(), this);
	vselect.setFilter(1, m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		if (m_inst->versionIsCustom())
		{
			auto reply = QMessageBox::question(
				this, tr("Revert?"),
				tr("This will revert any "
				   "changes you did to the version up to this point. Is that "
				   "OK?"),
				QMessageBox::Yes | QMessageBox::No);
			if (reply == QMessageBox::Yes)
			{
				m_inst->revertCustomVersion();
				m_inst->customizeVersion();
				{
					m_version = m_inst->getFullVersion();
					main_model->setSourceModel(m_version.data());
					updateVersionControls();
				}
			}
			else
				return;
		}
		else
		{
			m_inst->customizeVersion();
			m_version = m_inst->getFullVersion();
			main_model->setSourceModel(m_version.data());
			updateVersionControls();
		}
		ForgeVersionPtr forgeVersion = vselect.selectedVersion().dynamicCast<ForgeVersion>();
		if (!forgeVersion)
			return;
		auto entry = MMC->metacache()->resolveEntry("minecraftforge", forgeVersion->filename);
		if (entry->stale)
		{
			DownloadJob *fjob = new DownloadJob("Forge download");
			fjob->addCacheDownload(forgeVersion->installer_url, entry);
			ProgressDialog dlg(this);
			dlg.exec(fjob);
			if (dlg.result() == QDialog::Accepted)
			{
				// install
				QString forgePath = entry->getFullPath();
				ForgeInstaller forge(forgePath, forgeVersion->universal_url);
				if (!forge.apply(m_version))
				{
					// failure notice
				}
			}
			else
			{
				// failed to download forge :/
			}
		}
		else
		{
			// install
			QString forgePath = entry->getFullPath();
			ForgeInstaller forge(forgePath, forgeVersion->universal_url);
			if (!forge.apply(m_version))
			{
				// failure notice
			}
		}
	}
}

bool OneSixModEditDialog::loaderListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmModBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addModBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->loaderModTreeView, keyEvent);
}

bool OneSixModEditDialog::resourcePackListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmResPackBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addResPackBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->resPackTreeView, keyEvent);
}

bool OneSixModEditDialog::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QDialog::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->loaderModTreeView)
		return loaderListFilter(keyEvent);
	if (obj == ui->resPackTreeView)
		return resourcePackListFilter(keyEvent);
	return QDialog::eventFilter(obj, ev);
}

void OneSixModEditDialog::on_buttonBox_rejected()
{
	close();
}

void OneSixModEditDialog::on_addModBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(
		this, QApplication::translate("LegacyModEditDialog", "Select Loader Mods"));
	for (auto filename : fileNames)
	{
		m_mods->stopWatching();
		m_mods->installMod(QFileInfo(filename));
		m_mods->startWatching();
	}
}
void OneSixModEditDialog::on_rmModBtn_clicked()
{
	int first, last;
	auto list = ui->loaderModTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_mods->stopWatching();
	m_mods->deleteMods(first, last);
	m_mods->startWatching();
}
void OneSixModEditDialog::on_viewModBtn_clicked()
{
	openDirInDefaultProgram(m_inst->loaderModsDir(), true);
}

void OneSixModEditDialog::on_addResPackBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(
		this, QApplication::translate("LegacyModEditDialog", "Select Resource Packs"));
	for (auto filename : fileNames)
	{
		m_resourcepacks->stopWatching();
		m_resourcepacks->installMod(QFileInfo(filename));
		m_resourcepacks->startWatching();
	}
}
void OneSixModEditDialog::on_rmResPackBtn_clicked()
{
	int first, last;
	auto list = ui->resPackTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_resourcepacks->stopWatching();
	m_resourcepacks->deleteMods(first, last);
	m_resourcepacks->startWatching();
}
void OneSixModEditDialog::on_viewResPackBtn_clicked()
{
	openDirInDefaultProgram(m_inst->resourcePacksDir(), true);
}
