/* Copyright 2013-2015 MultiMC Contributors
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

#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>

#include "VersionPage.h"
#include "ui_VersionPage.h"

#include "dialogs/CustomMessageBox.h"
#include "dialogs/VersionSelectDialog.h"
#include "dialogs/ModEditDialogCommon.h"

#include "dialogs/ProgressDialog.h"
#include <GuiUtil.h>

#include <QAbstractItemModel>
#include <QMessageBox>
#include <QListView>
#include <QString>
#include <QUrl>

#include "minecraft/MinecraftProfile.h"
#include "forge/ForgeVersionList.h"
#include "forge/ForgeInstaller.h"
#include "liteloader/LiteLoaderVersionList.h"
#include "liteloader/LiteLoaderInstaller.h"
#include "auth/MojangAccountList.h"
#include "minecraft/Mod.h"
#include <minecraft/MinecraftVersion.h>
#include <minecraft/MinecraftVersionList.h>
#include "icons/IconList.h"
#include "Exception.h"

QIcon VersionPage::icon() const
{
	return ENV.icons()->getIcon(m_inst->iconKey());
}
bool VersionPage::shouldDisplay() const
{
	return !m_inst->isRunning();
}

void VersionPage::setParentContainer(BasePageContainer * container)
{
	m_container = container;
}

VersionPage::VersionPage(OneSixInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::VersionPage), m_inst(inst)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	reloadMinecraftProfile();

	m_version = m_inst->getMinecraftProfile();
	if (m_version)
	{
		ui->packageView->setModel(m_version.get());
		ui->packageView->installEventFilter(this);
		ui->packageView->setSelectionMode(QAbstractItemView::SingleSelection);
		connect(ui->packageView->selectionModel(), &QItemSelectionModel::currentChanged,
				this, &VersionPage::versionCurrent);
		updateVersionControls();
		// select first item.
		preselect(0);
	}
	else
	{
		disableVersionControls();
	}
	connect(m_inst, &OneSixInstance::versionReloaded, this,
			&VersionPage::updateVersionControls);
}

VersionPage::~VersionPage()
{
	delete ui;
}

void VersionPage::updateVersionControls()
{
	ui->forgeBtn->setEnabled(true);
	ui->liteloaderBtn->setEnabled(true);
	updateButtons();
}

void VersionPage::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->reloadBtn->setEnabled(false);
	updateButtons();
}

bool VersionPage::reloadMinecraftProfile()
{
	try
	{
		m_inst->reloadProfile();
		return true;
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
		return false;
	}
	catch (...)
	{
		QMessageBox::critical(
			this, tr("Error"),
			tr("Couldn't load the instance profile."));
		return false;
	}
}

void VersionPage::on_reloadBtn_clicked()
{
	reloadMinecraftProfile();
}

void VersionPage::on_removeBtn_clicked()
{
	if (ui->packageView->currentIndex().isValid())
	{
		// FIXME: use actual model, not reloading.
		if (!m_version->remove(ui->packageView->currentIndex().row()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't remove file"));
		}
	}
	updateButtons();
}

void VersionPage::on_modBtn_clicked()
{
	if(m_container)
	{
		m_container->selectPage("mods");
	}
}

void VersionPage::on_jarmodBtn_clicked()
{
	bool nagShown = false;
	auto traits = m_version->traits;
	if (!traits.contains("legacyLaunch") && !traits.contains("alphaLaunch"))
	{
		// not legacy launch... nag
		auto seenNag = MMC->settings()->get("JarModNagSeen").toBool();
		if(!seenNag)
		{
			auto result = QMessageBox::question(this,
				tr("Are you sure?"),
				tr("This will add mods directly to the Minecraft jar.\n"
					"Unless you KNOW that this is what NEEDS to be done, you should just use the mods folder (Loader mods).\n"
					"\n"
					"Do you want to continue?"),
					tr("I understand, continue."), tr("Cancel"), QString(), 1, 1
				);
			if(result != 0)
				return;
			nagShown = true;
		}
	}
	auto list = GuiUtil::BrowseForFiles("jarmod", tr("Select jar mods"), tr("Minecraft.jar mods (*.zip *.jar)"), MMC->settings()->get("CentralModsDir").toString(), this->parentWidget());
	if(!list.empty())
	{
		m_version->installJarMods(list);
		if(nagShown)
		{
			MMC->settings()->set("JarModNagSeen", QVariant(true));
		}
	}
	updateButtons();
}

void VersionPage::on_resetOrderBtn_clicked()
{
	try
	{
		m_version->resetOrder();
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
	updateButtons();
}

void VersionPage::on_moveUpBtn_clicked()
{
	try
	{
		m_version->move(currentRow(), MinecraftProfile::MoveUp);
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
	updateButtons();
}

void VersionPage::on_moveDownBtn_clicked()
{
	try
	{
		m_version->move(currentRow(), MinecraftProfile::MoveDown);
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
	updateButtons();
}

void VersionPage::on_changeVersionBtn_clicked()
{
	VersionSelectDialog vselect(m_inst->versionList().get(), tr("Change Minecraft version"),
								this);
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

	if (!m_version->isVanilla())
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
		reloadMinecraftProfile();
	}
	m_inst->setIntendedVersionId(vselect.selectedVersion()->descriptor());
	doUpdate();
}

int VersionPage::doUpdate()
{
	auto updateTask = m_inst->createUpdateTask();
	if (!updateTask)
	{
		return 1;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	int ret = tDialog.execWithTask(updateTask.get());
	updateButtons();
	return ret;
}

void VersionPage::on_forgeBtn_clicked()
{
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setExactFilter(BaseVersionList::ParentGameVersionRole, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	vselect.setEmptyErrorString(tr("Couldn't load or download the Forge version lists!"));
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.execWithTask(
			ForgeInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
		preselect(m_version->rowCount(QModelIndex())-1);
	}
}

void VersionPage::on_liteloaderBtn_clicked()
{
	VersionSelectDialog vselect(MMC->liteloaderlist().get(), tr("Select LiteLoader version"),
								this);
	vselect.setExactFilter(BaseVersionList::ParentGameVersionRole, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	vselect.setEmptyErrorString(tr("Couldn't load or download the LiteLoader version lists!"));
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.execWithTask(
			LiteLoaderInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
		preselect(m_version->rowCount(QModelIndex())-1);
	}
}

void VersionPage::versionCurrent(const QModelIndex &current, const QModelIndex &previous)
{
	currentIdx = current.row();
	updateButtons(currentIdx);
}

void VersionPage::preselect(int row)
{
	if(row < 0)
	{
		row = 0;
	}
	if(row >= m_version->rowCount(QModelIndex()))
	{
		row = m_version->rowCount(QModelIndex()) - 1;
	}
	if(row < 0)
	{
		return;
	}
	auto model_index = m_version->index(row);
	ui->packageView->selectionModel()->select(model_index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
	updateButtons(row);
}

void VersionPage::updateButtons(int row)
{
	if(row == -1)
		row = currentRow();
	auto patch = m_version->versionPatch(row);
	if (!patch)
	{
		ui->removeBtn->setDisabled(true);
		ui->moveDownBtn->setDisabled(true);
		ui->moveUpBtn->setDisabled(true);
		ui->changeVersionBtn->setDisabled(true);
		ui->editBtn->setDisabled(true);
		ui->customizeBtn->setDisabled(true);
		ui->revertBtn->setDisabled(true);
	}
	else
	{
		ui->removeBtn->setEnabled(patch->isRemovable());
		ui->moveDownBtn->setEnabled(patch->isMoveable());
		ui->moveUpBtn->setEnabled(patch->isMoveable());
		ui->changeVersionBtn->setEnabled(patch->isVersionChangeable());
		ui->editBtn->setEnabled(patch->isEditable());
		ui->customizeBtn->setEnabled(patch->isCustomizable());
		ui->revertBtn->setEnabled(patch->isRevertible());
	}
}

void VersionPage::onGameUpdateError(QString error)
{
	CustomMessageBox::selectable(this, tr("Error updating instance"), error,
								 QMessageBox::Warning)->show();
}

ProfilePatchPtr VersionPage::current()
{
	auto row = currentRow();
	if(row < 0)
	{
		return nullptr;
	}
	return m_version->versionPatch(row);
}

int VersionPage::currentRow()
{
	if (ui->packageView->selectionModel()->selectedRows().isEmpty())
	{
		return -1;
	}
	return ui->packageView->selectionModel()->selectedRows().first().row();
}

void VersionPage::on_customizeBtn_clicked()
{
	auto version = currentRow();
	if(version == -1)
	{
		return;
	}
	//HACK HACK remove, this is dumb
	auto patch = m_version->versionPatch(version);
	auto mc = std::dynamic_pointer_cast<MinecraftVersion>(patch);
	if(mc && mc->needsUpdate())
	{
		if(!doUpdate())
		{
			return;
		}
	}
	if(!m_version->customize(version))
	{
		// TODO: some error box here
	}
	updateButtons();
	preselect(currentIdx);
}

void VersionPage::on_editBtn_clicked()
{
	auto version = current();
	if(!version)
	{
		return;
	}
	auto filename = version->getPatchFilename();
	if(!QFileInfo::exists(filename))
	{
		qWarning() << "file" << filename << "can't be opened for editing, doesn't exist!";
		return;
	}
	MMC->openJsonEditor(filename);
}

void VersionPage::on_revertBtn_clicked()
{
	auto version = currentRow();
	if(version == -1)
	{
		return;
	}
	auto mcraw = MMC->minecraftlist()->findVersion(m_inst->intendedVersionId());
	auto mc = std::dynamic_pointer_cast<MinecraftVersion>(mcraw);
	if(mc && mc->needsUpdate())
	{
		if(!doUpdate())
		{
			return;
		}
	}
	if(!m_version->revertToBase(version))
	{
		// TODO: some error box here
	}
	updateButtons();
	preselect(currentIdx);
}
