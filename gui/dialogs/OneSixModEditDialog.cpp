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
#include "logic/OneSixVersion.h"
#include "logic/EnabledItemFilter.h"
#include "logic/lists/ForgeVersionList.h"
#include "logic/lists/LiteLoaderVersionList.h"
#include "logic/ForgeInstaller.h"
#include "logic/LiteLoaderInstaller.h"
#include "logic/OneSixVersionBuilder.h"

template<typename A, typename B>
QMap<A, B> invert(const QMap<B, A> &in)
{
	QMap<A, B> out;
	for (auto it = in.begin(); it != in.end(); ++it)
	{
		out.insert(it.value(), it.key());
	}
	return out;
}

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

	connect(m_inst, &OneSixInstance::versionReloaded, this, &OneSixModEditDialog::updateVersionControls);
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
	ui->mainClassEdit->setText(m_version->mainClass);
}

void OneSixModEditDialog::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->reloadLibrariesBtn->setEnabled(false);
	ui->removeLibraryBtn->setEnabled(false);
	ui->mainClassEdit->setText("");
}

void OneSixModEditDialog::on_reloadLibrariesBtn_clicked()
{
	m_inst->reloadVersion(this);
}

void OneSixModEditDialog::on_removeLibraryBtn_clicked()
{
	if (ui->libraryTreeView->currentIndex().isValid())
	{
		if (!m_version->remove(ui->libraryTreeView->currentIndex().row()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't remove file"));
		}
		else
		{
			m_inst->reloadVersion(this);
		}
	}
}

void OneSixModEditDialog::on_resetLibraryOrderBtn_clicked()
{
	QDir(m_inst->instanceRoot()).remove("order.json");
	m_inst->reloadVersion(this);
}
void OneSixModEditDialog::on_moveLibraryUpBtn_clicked()
{

	QMap<QString, int> order = getExistingOrder();
	if (order.size() < 2 || ui->libraryTreeView->selectionModel()->selectedIndexes().isEmpty())
	{
		return;
	}
	const int ourRow = ui->libraryTreeView->selectionModel()->selectedIndexes().first().row();
	const QString ourId = m_version->versionFileId(ourRow);
	const int ourOrder = order[ourId];
	if (ourId.isNull() || ourId.startsWith("org.multimc."))
	{
		return;
	}

	QMap<int, QString> sortedOrder = invert(order);

	QList<int> sortedOrders = sortedOrder.keys();
	const int ourIndex = sortedOrders.indexOf(ourOrder);
	if (ourIndex <= 0)
	{
		return;
	}
	const int ourNewOrder = sortedOrders.at(ourIndex - 1);
	order[ourId] = ourNewOrder;
	order[sortedOrder[sortedOrders[ourIndex - 1]]] = ourOrder;

	if (!OneSixVersionBuilder::writeOverrideOrders(order, m_inst))
	{
		QMessageBox::critical(this, tr("Error"), tr("Couldn't save the new order"));
	}
	else
	{
		m_inst->reloadVersion(this);
		ui->libraryTreeView->selectionModel()->select(m_version->index(ourRow - 1), QItemSelectionModel::SelectCurrent);
	}
}
void OneSixModEditDialog::on_moveLibraryDownBtn_clicked()
{
	QMap<QString, int> order = getExistingOrder();
	if (order.size() < 2 || ui->libraryTreeView->selectionModel()->selectedIndexes().isEmpty())
	{
		return;
	}
	const int ourRow = ui->libraryTreeView->selectionModel()->selectedIndexes().first().row();
	const QString ourId = m_version->versionFileId(ourRow);
	const int ourOrder = order[ourId];
	if (ourId.isNull() || ourId.startsWith("org.multimc."))
	{
		return;
	}

	QMap<int, QString> sortedOrder = invert(order);

	QList<int> sortedOrders = sortedOrder.keys();
	const int ourIndex = sortedOrders.indexOf(ourOrder);
	if ((ourIndex + 1) >= sortedOrders.size())
	{
		return;
	}
	const int ourNewOrder = sortedOrders.at(ourIndex + 1);
	order[ourId] = ourNewOrder;
	order[sortedOrder[sortedOrders[ourIndex + 1]]] = ourOrder;

	if (!OneSixVersionBuilder::writeOverrideOrders(order, m_inst))
	{
		QMessageBox::critical(this, tr("Error"), tr("Couldn't save the new order"));
	}
	else
	{
		m_inst->reloadVersion(this);
		ui->libraryTreeView->selectionModel()->select(m_version->index(ourRow + 1), QItemSelectionModel::SelectCurrent);
	}
}

void OneSixModEditDialog::on_forgeBtn_clicked()
{
	if (QDir(m_inst->instanceRoot()).exists("custom.json"))
	{
		if (QMessageBox::question(this, tr("Revert?"), tr("This action will remove your custom.json. Continue?")) != QMessageBox::Yes)
		{
			return;
		}
		QDir(m_inst->instanceRoot()).remove("custom.json");
		m_inst->reloadVersion(this);
	}
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setFilter(1, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
							  m_inst->currentVersionId());
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
				if (!forge.add(m_inst))
				{
					QLOG_ERROR() << "Failure installing forge";
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
			if (!forge.add(m_inst))
			{
				QLOG_ERROR() << "Failure installing forge";
			}
		}
	}
	m_inst->reloadVersion(this);
}

void OneSixModEditDialog::on_liteloaderBtn_clicked()
{
	if (QDir(m_inst->instanceRoot()).exists("custom.json"))
	{
		if (QMessageBox::question(this, tr("Revert?"), tr("This action will remove your custom.json. Continue?")) != QMessageBox::Yes)
		{
			return;
		}
		QDir(m_inst->instanceRoot()).remove("custom.json");
		m_inst->reloadVersion(this);
	}
	VersionSelectDialog vselect(MMC->liteloaderlist().get(), tr("Select LiteLoader version"), this);
	vselect.setFilter(1, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
							  m_inst->currentVersionId());
	if (vselect.exec() && vselect.selectedVersion())
	{
		LiteLoaderVersionPtr liteloaderVersion =
				std::dynamic_pointer_cast<LiteLoaderVersion>(vselect.selectedVersion());
		if (!liteloaderVersion)
			return;
		LiteLoaderInstaller liteloader(liteloaderVersion);
		if (!liteloader.add(m_inst))
		{
			QMessageBox::critical(this, tr("LiteLoader"),
								  tr("For reasons unknown, the LiteLoader installation failed. "
									 "Check your MultiMC log files for details."));
		}
		else
		{
			m_inst->reloadVersion(this);
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

QMap<QString, int> OneSixModEditDialog::getExistingOrder() const
{

	QMap<QString, int> order;
	// default
	{
		for (OneSixVersion::VersionFile file : m_version->versionFiles)
		{
			if (file.id.startsWith("org.multimc."))
			{
				continue;
			}
			order.insert(file.id, file.order);
		}
	}
	// overriden
	{
		QMap<QString, int> overridenOrder = OneSixVersionBuilder::readOverrideOrders(m_inst);
		for (auto id : order.keys())
		{
			if (overridenOrder.contains(id))
			{
				order[id] = overridenOrder[id];
			}
		}
	}
	return order;
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

void OneSixModEditDialog::versionCurrent(const QModelIndex &current, const QModelIndex &previous)
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
