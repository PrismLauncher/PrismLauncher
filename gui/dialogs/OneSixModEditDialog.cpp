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

#include "OneSixModEditDialog.h"
#include "ModEditDialogCommon.h"
#include "ui_OneSixModEditDialog.h"

#include "gui/Platform.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/VersionSelectDialog.h"

#include "gui/dialogs/ProgressDialog.h"

#include "logic/ModList.h"
#include "logic/VersionFinal.h"
#include "logic/EnabledItemFilter.h"
#include "logic/lists/ForgeVersionList.h"
#include "logic/lists/LiteLoaderVersionList.h"
#include "logic/ForgeInstaller.h"
#include "logic/LiteLoaderInstaller.h"
#include "logic/OneSixVersionBuilder.h"

OneSixModEditDialog::OneSixModEditDialog(OneSixInstance *inst, QWidget *parent)
	: QDialog(parent), ui(new Ui::OneSixModEditDialog), m_inst(inst)
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
		connect(ui->libraryTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
				this, &OneSixModEditDialog::versionCurrent);
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

	connect(m_inst, &OneSixInstance::versionReloaded, this,
			&OneSixModEditDialog::updateVersionControls);
}

OneSixModEditDialog::~OneSixModEditDialog()
{
	m_mods->stopWatching();
	m_resourcepacks->stopWatching();
	delete ui;
}

void OneSixModEditDialog::updateVersionControls()
{
	ui->forgeBtn->setEnabled(true);
	ui->liteloaderBtn->setEnabled(true);
}

void OneSixModEditDialog::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->reloadLibrariesBtn->setEnabled(false);
	ui->removeLibraryBtn->setEnabled(false);
}

bool OneSixModEditDialog::reloadInstanceVersion()
{
	try
	{
		m_inst->reloadVersion();
		return true;
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
		return false;
	}
	catch (...)
	{
		QMessageBox::critical(
			this, tr("Error"),
			tr("Failed to load the version description file for reasons unknown."));
		return false;
	}
}

void OneSixModEditDialog::on_reloadLibrariesBtn_clicked()
{
	reloadInstanceVersion();
}

void OneSixModEditDialog::on_removeLibraryBtn_clicked()
{
	if (ui->libraryTreeView->currentIndex().isValid())
	{
		// FIXME: use actual model, not reloading.
		if (!m_version->remove(ui->libraryTreeView->currentIndex().row()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't remove file"));
		}
	}
}

void OneSixModEditDialog::on_resetLibraryOrderBtn_clicked()
{
	try
	{
		m_version->resetOrder();
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void OneSixModEditDialog::on_moveLibraryUpBtn_clicked()
{
	if (ui->libraryTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return;
	}
	try
	{
		const int row = ui->libraryTreeView->selectionModel()->selectedRows().first().row();
		const int newRow = 0;m_version->move(row, VersionFinal::MoveUp);
		//ui->libraryTreeView->selectionModel()->setCurrentIndex(m_version->index(newRow), QItemSelectionModel::ClearAndSelect);
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void OneSixModEditDialog::on_moveLibraryDownBtn_clicked()
{
	if (ui->libraryTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return;
	}
	try
	{
		const int row = ui->libraryTreeView->selectionModel()->selectedRows().first().row();
		const int newRow = 0;m_version->move(row, VersionFinal::MoveDown);
		//ui->libraryTreeView->selectionModel()->setCurrentIndex(m_version->index(newRow), QItemSelectionModel::ClearAndSelect);
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void OneSixModEditDialog::on_forgeBtn_clicked()
{
	// FIXME: use actual model, not reloading. Move logic to model.
	if (m_version->hasFtbPack())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove the FTB pack version patch. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->removeFtbPack();
		reloadInstanceVersion();
	}
	if (m_version->isCustom())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove your custom.json. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->revertToBase();
		reloadInstanceVersion();
	}
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setExactFilter(1, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.exec(ForgeInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
	}
}

void OneSixModEditDialog::on_liteloaderBtn_clicked()
{
	if (m_version->hasFtbPack())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove the FTB pack version patch. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->removeFtbPack();
		reloadInstanceVersion();
	}
	if (m_version->isCustom())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove your custom.json. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->revertToBase();
		reloadInstanceVersion();
	}
	VersionSelectDialog vselect(MMC->liteloaderlist().get(), tr("Select LiteLoader version"),
								this);
	vselect.setExactFilter(1, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.exec(LiteLoaderInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
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

void OneSixModEditDialog::loaderCurrent(QModelIndex current, QModelIndex previous)
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

void OneSixModEditDialog::versionCurrent(const QModelIndex &current,
										 const QModelIndex &previous)
{
	if (!current.isValid())
	{
		ui->removeLibraryBtn->setDisabled(true);
	}
	else
	{
		ui->removeLibraryBtn->setEnabled(m_version->canRemove(current.row()));
	}
}
