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

#include "InstanceEditDialog.h"
#include "ui_InstanceEditDialog.h"

#include "gui/Platform.h"
#include "gui/dialogs/CustomMessageBox.h"
#include "gui/dialogs/VersionSelectDialog.h"

#include "gui/dialogs/ProgressDialog.h"
#include "InstanceSettings.h"

#include "logic/ModList.h"
#include "logic/VersionFinal.h"
#include "logic/EnabledItemFilter.h"
#include "logic/forge/ForgeVersionList.h"
#include "logic/forge/ForgeInstaller.h"
#include "logic/liteloader/LiteLoaderVersionList.h"
#include "logic/liteloader/LiteLoaderInstaller.h"
#include "logic/OneSixVersionBuilder.h"
#include "logic/auth/MojangAccountList.h"

#include <QAbstractItemModel>
#include <logic/Mod.h>

#include "CustomMessageBox.h"
#include <QDesktopServices>
#include <QMessageBox>
#include <QString>
#include <QUrl>

bool lastfirst(QModelIndexList &list, int &first, int &last)
{
	if (!list.size())
		return false;
	first = last = list[0].row();
	for (auto item : list)
	{
		int row = item.row();
		if (row < first)
			first = row;
		if (row > last)
			last = row;
	}
	return true;
}

void showWebsiteForMod(QWidget *parentDlg, Mod &m)
{
	QString url = m.homeurl();
	if (url.size())
	{
		// catch the cases where the protocol is missing
		if (!url.startsWith("http"))
		{
			url = "http://" + url;
		}
		QDesktopServices::openUrl(url);
	}
	else
	{
		CustomMessageBox::selectable(
			parentDlg, QObject::tr("How sad!"),
			QObject::tr("The mod author didn't provide a website link for this mod."),
			QMessageBox::Warning);
	}
}

InstanceEditDialog::InstanceEditDialog(OneSixInstance *inst, QWidget *parent)
	: QDialog(parent), ui(new Ui::InstanceEditDialog), m_inst(inst)
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
				this, &InstanceEditDialog::versionCurrent);
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
	// Core mods
	{
		ensureFolderPathExists(m_inst->coreModsDir());
		m_coremods = m_inst->coreModList();
		ui->coreModsTreeView->setModel(m_coremods.get());
		ui->coreModsTreeView->installEventFilter(this);
		m_coremods->startWatching();
		auto smodel = ui->coreModsTreeView->selectionModel();
		connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
				SLOT(coreCurrent(QModelIndex, QModelIndex)));
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
			&InstanceEditDialog::updateVersionControls);
}

InstanceEditDialog::~InstanceEditDialog()
{
	m_mods->stopWatching();
	m_resourcepacks->stopWatching();
	m_coremods->stopWatching();
	delete ui;
}

void InstanceEditDialog::updateVersionControls()
{
	ui->forgeBtn->setEnabled(true);
	ui->liteloaderBtn->setEnabled(true);
}

void InstanceEditDialog::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->reloadLibrariesBtn->setEnabled(false);
	ui->removeLibraryBtn->setEnabled(false);
}

bool InstanceEditDialog::reloadInstanceVersion()
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

void InstanceEditDialog::on_settingsBtn_clicked()
{
	InstanceSettings settings(&m_inst->settings(), this);
	settings.setWindowTitle(tr("Instance settings"));
	settings.exec();
}

void InstanceEditDialog::on_reloadLibrariesBtn_clicked()
{
	reloadInstanceVersion();
}

void InstanceEditDialog::on_removeLibraryBtn_clicked()
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

void InstanceEditDialog::on_resetLibraryOrderBtn_clicked()
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

void InstanceEditDialog::on_moveLibraryUpBtn_clicked()
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

void InstanceEditDialog::on_moveLibraryDownBtn_clicked()
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

void InstanceEditDialog::on_changeMCVersionBtn_clicked()
{
	VersionSelectDialog vselect(m_inst->versionList().get(), tr("Change Minecraft version"), this);
	if (!vselect.exec() || !vselect.selectedVersion())
		return;

	if (!MMC->accounts()->anyAccountIsValid())
	{
		CustomMessageBox::selectable(
			this, tr("Error"),
			tr("MultiMC cannot download Minecraft or update instances unless you have at least "
			   "one account added.\nPlease add your Mojang or Minecraft account."),
			QMessageBox::Warning)->show();
		return;
	}

	if (m_inst->versionIsCustom())
	{
		auto result = CustomMessageBox::selectable(
			this, tr("Are you sure?"),
			tr("This will remove any library/version customization you did previously. "
			   "This includes things like Forge install and similar."),
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Abort,
			QMessageBox::Abort)->exec();

		if (result != QMessageBox::Ok)
			return;
		m_version->revertToVanilla();
		reloadInstanceVersion();
	}
	m_inst->setIntendedVersionId(vselect.selectedVersion()->descriptor());

	auto updateTask = m_inst->doUpdate();
	if (!updateTask)
	{
		return;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	tDialog.exec(updateTask.get());
}

void InstanceEditDialog::on_forgeBtn_clicked()
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
	if (m_version->usesLegacyCustomJson())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove your custom.json. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->revertToVanilla();
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

void InstanceEditDialog::on_liteloaderBtn_clicked()
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
	if (m_version->usesLegacyCustomJson())
	{
		if (QMessageBox::question(this, tr("Revert?"),
								  tr("This action will remove your custom.json. Continue?")) !=
			QMessageBox::Yes)
		{
			return;
		}
		m_version->revertToVanilla();
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

bool InstanceEditDialog::loaderListFilter(QKeyEvent *keyEvent)
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

bool InstanceEditDialog::coreListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmCoreBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addCoreBtn_clicked();
		return true;
	default:
		break;
	}
	return QDialog::eventFilter(ui->coreModsTreeView, keyEvent);
}

bool InstanceEditDialog::resourcePackListFilter(QKeyEvent *keyEvent)
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

bool InstanceEditDialog::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QDialog::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->loaderModTreeView)
		return loaderListFilter(keyEvent);
	if (obj == ui->coreModsTreeView)
		return coreListFilter(keyEvent);
	if (obj == ui->resPackTreeView)
		return resourcePackListFilter(keyEvent);
	return QDialog::eventFilter(obj, ev);
}

void InstanceEditDialog::on_buttonBox_rejected()
{
	close();
}

void InstanceEditDialog::on_addModBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(
		this, QApplication::translate("InstanceEditDialog", "Select Loader Mods"));
	for (auto filename : fileNames)
	{
		m_mods->stopWatching();
		m_mods->installMod(QFileInfo(filename));
		m_mods->startWatching();
	}
}
void InstanceEditDialog::on_rmModBtn_clicked()
{
	int first, last;
	auto list = ui->loaderModTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_mods->stopWatching();
	m_mods->deleteMods(first, last);
	m_mods->startWatching();
}
void InstanceEditDialog::on_viewModBtn_clicked()
{
	openDirInDefaultProgram(m_inst->loaderModsDir(), true);
}

void InstanceEditDialog::on_addCoreBtn_clicked()
{
	//: Title of core mod selection dialog
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Core Mods"));
	for (auto filename : fileNames)
	{
		m_coremods->stopWatching();
		m_coremods->installMod(QFileInfo(filename));
		m_coremods->startWatching();
	}
}

void InstanceEditDialog::on_rmCoreBtn_clicked()
{
	int first, last;
	auto list = ui->coreModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_coremods->stopWatching();
	m_coremods->deleteMods(first, last);
	m_coremods->startWatching();
}

void InstanceEditDialog::on_viewCoreBtn_clicked()
{
	openDirInDefaultProgram(m_inst->coreModsDir(), true);
}

void InstanceEditDialog::on_addResPackBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(
		this, QApplication::translate("InstanceEditDialog", "Select Resource Packs"));
	for (auto filename : fileNames)
	{
		m_resourcepacks->stopWatching();
		m_resourcepacks->installMod(QFileInfo(filename));
		m_resourcepacks->startWatching();
	}
}
void InstanceEditDialog::on_rmResPackBtn_clicked()
{
	int first, last;
	auto list = ui->resPackTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_resourcepacks->stopWatching();
	m_resourcepacks->deleteMods(first, last);
	m_resourcepacks->startWatching();
}
void InstanceEditDialog::on_viewResPackBtn_clicked()
{
	openDirInDefaultProgram(m_inst->resourcePacksDir(), true);
}

void InstanceEditDialog::loaderCurrent(QModelIndex current, QModelIndex previous)
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

void InstanceEditDialog::versionCurrent(const QModelIndex &current,
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

void InstanceEditDialog::coreCurrent(QModelIndex current, QModelIndex previous)
{
	if (!current.isValid())
	{
		ui->coreMIFrame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_coremods->operator[](row);
	ui->coreMIFrame->updateWithMod(m);
}
