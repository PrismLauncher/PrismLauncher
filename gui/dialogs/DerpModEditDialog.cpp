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

#include <pathutils.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QDesktopServices>

#include "DerpModEditDialog.h"
#include "ModEditDialogCommon.h"
#include "ui_DerpModEditDialog.h"

#include "gui/Platform.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/VersionSelectDialog.h"

#include "gui/dialogs/ProgressDialog.h"

#include "logic/ModList.h"
#include "logic/DerpVersion.h"
#include "logic/EnabledItemFilter.h"
#include "logic/lists/ForgeVersionList.h"
#include "logic/ForgeInstaller.h"
#include "logic/LiteLoaderInstaller.h"

DerpModEditDialog::DerpModEditDialog(DerpInstance *inst, QWidget *parent)
	: QDialog(parent), ui(new Ui::DerpModEditDialog), m_inst(inst)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	// libraries!

	m_version = m_inst->getFullVersion();
	if (m_version)
	{
		main_model = new EnabledItemFilter(this);
		main_model->setActive(true);
		main_model->setSourceModel(m_version.get());
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
		ui->loaderModTreeView->setModel(m_mods.get());
		ui->loaderModTreeView->installEventFilter(this);
		m_mods->startWatching();
		auto smodel = ui->loaderModTreeView->selectionModel();
		connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
				SLOT(loaderCurrent(QModelIndex, QModelIndex)));
	}
	// resource packs
	{
		ensureFolderPathExists(m_inst->resourcePacksDir());
		m_resourcepacks = m_inst->resourcePackList();
		ui->resPackTreeView->setModel(m_resourcepacks.get());
		ui->resPackTreeView->installEventFilter(this);
		m_resourcepacks->startWatching();
	}

	connect(m_inst, &DerpInstance::versionReloaded, this, &DerpModEditDialog::updateVersionControls);
}

DerpModEditDialog::~DerpModEditDialog()
{
	m_mods->stopWatching();
	m_resourcepacks->stopWatching();
	delete ui;
}

void DerpModEditDialog::updateVersionControls()
{
	bool customVersion = m_inst->versionIsCustom();
	ui->forgeBtn->setEnabled(true);
	ui->liteloaderBtn->setEnabled(LiteLoaderInstaller(m_inst->intendedVersionId()).canApply());
	ui->customEditorBtn->setEnabled(customVersion);
}

void DerpModEditDialog::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->customEditorBtn->setEnabled(false);
}

void DerpModEditDialog::on_customEditorBtn_clicked()
{
	if (QDir(m_inst->instanceRoot()).exists("custom.json"))
	{
		if (!MMC->openJsonEditor(m_inst->instanceRoot() + "/custom.json"))
		{
			QMessageBox::warning(this, tr("Error"), tr("Unable to open custom.json, check the settings"));
		}
	}
}

void DerpModEditDialog::on_forgeBtn_clicked()
{
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setFilter(1, m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		ForgeVersionPtr forgeVersion =
			std::dynamic_pointer_cast<ForgeVersion>(vselect.selectedVersion());
		if (!forgeVersion)
			return;
		auto entry = MMC->metacache()->resolveEntry("minecraftforge", forgeVersion->filename);
		if (entry->stale)
		{
			NetJob *fjob = new NetJob("Forge download");
			fjob->addNetAction(CacheDownload::make(forgeVersion->installer_url, entry));
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

void DerpModEditDialog::on_liteloaderBtn_clicked()
{
	LiteLoaderInstaller liteloader(m_inst->intendedVersionId());
	if (!liteloader.canApply())
	{
		QMessageBox::critical(
			this, tr("LiteLoader"),
			tr("There is no information available on how to install LiteLoader "
			   "into this version of Minecraft"));
		return;
	}
	if (!liteloader.apply(m_version))
	{
		QMessageBox::critical(
			this, tr("LiteLoader"),
			tr("For reasons unknown, the LiteLoader installation failed. Check your MultiMC log files for details."));
	}
}

bool DerpModEditDialog::loaderListFilter(QKeyEvent *keyEvent)
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

bool DerpModEditDialog::resourcePackListFilter(QKeyEvent *keyEvent)
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

bool DerpModEditDialog::eventFilter(QObject *obj, QEvent *ev)
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

void DerpModEditDialog::on_buttonBox_rejected()
{
	close();
}

void DerpModEditDialog::on_addModBtn_clicked()
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
void DerpModEditDialog::on_rmModBtn_clicked()
{
	int first, last;
	auto list = ui->loaderModTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_mods->stopWatching();
	m_mods->deleteMods(first, last);
	m_mods->startWatching();
}
void DerpModEditDialog::on_viewModBtn_clicked()
{
	openDirInDefaultProgram(m_inst->loaderModsDir(), true);
}

void DerpModEditDialog::on_addResPackBtn_clicked()
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
void DerpModEditDialog::on_rmResPackBtn_clicked()
{
	int first, last;
	auto list = ui->resPackTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_resourcepacks->stopWatching();
	m_resourcepacks->deleteMods(first, last);
	m_resourcepacks->startWatching();
}
void DerpModEditDialog::on_viewResPackBtn_clicked()
{
	openDirInDefaultProgram(m_inst->resourcePacksDir(), true);
}

void DerpModEditDialog::loaderCurrent(QModelIndex current, QModelIndex previous)
{
	if (!current.isValid())
	{
		ui->frame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_mods->operator[](row);
	ui->frame->updateWithMod(m);
}
