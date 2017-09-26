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
#include "minecraft/auth/MojangAccountList.h"
#include "minecraft/Mod.h"
#include "icons/IconList.h"
#include "Exception.h"

#include "MultiMC.h"

#include <meta/Index.h>
#include <meta/VersionList.h>

class IconProxy : public QIdentityProxyModel
{
	Q_OBJECT
public:

	IconProxy(QWidget *parentWidget) : QIdentityProxyModel(parentWidget)
	{
		connect(parentWidget, &QObject::destroyed, this, &IconProxy::widgetGone);
		m_parentWidget = parentWidget;
	}

	virtual QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const override
	{
		QVariant var = QIdentityProxyModel::data(mapToSource(proxyIndex), role);
		int column = proxyIndex.column();
		if(column == 0 && role == Qt::DecorationRole && m_parentWidget)
		{
			if(!var.isNull())
			{
				auto string = var.toString();
				if(string == "warning")
				{
					return MMC->getThemedIcon("status-yellow");
				}
				else if(string == "error")
				{
					return MMC->getThemedIcon("status-bad");
				}
			}
			return MMC->getThemedIcon("status-good");
		}
		return var;
	}
private slots:
	void widgetGone()
	{
		m_parentWidget = nullptr;
	}

private:
	QWidget *m_parentWidget = nullptr;
};

QIcon VersionPage::icon() const
{
	return MMC->icons()->getIcon(m_inst->iconKey());
}
bool VersionPage::shouldDisplay() const
{
	return !m_inst->isRunning();
}

VersionPage::VersionPage(MinecraftInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::VersionPage), m_inst(inst)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	reloadMinecraftProfile();

	m_profile = m_inst->getMinecraftProfile();
	if (m_profile)
	{
		auto proxy = new IconProxy(ui->packageView);
		proxy->setSourceModel(m_profile.get());
		ui->packageView->setModel(proxy);
		ui->packageView->installEventFilter(this);
		ui->packageView->setSelectionMode(QAbstractItemView::SingleSelection);
		connect(ui->packageView->selectionModel(), &QItemSelectionModel::currentChanged, this, &VersionPage::versionCurrent);
		auto smodel = ui->packageView->selectionModel();
		connect(smodel, &QItemSelectionModel::currentChanged, this, &VersionPage::packageCurrent);
		updateVersionControls();
		// select first item.
		preselect(0);
	}
	else
	{
		disableVersionControls();
	}
	connect(m_inst, &MinecraftInstance::versionReloaded, this,
			&VersionPage::updateVersionControls);
}

VersionPage::~VersionPage()
{
	delete ui;
}

void VersionPage::packageCurrent(const QModelIndex &current, const QModelIndex &previous)
{
	if (!current.isValid())
	{
		ui->frame->clear();
		return;
	}
	int row = current.row();
	auto patch = m_profile->versionPatch(row);
	auto severity = patch->getProblemSeverity();
	switch(severity)
	{
		case ProblemSeverity::Warning:
			ui->frame->setModText(tr("%1 possibly has issues.").arg(patch->getName()));
			break;
		case ProblemSeverity::Error:
			ui->frame->setModText(tr("%1 has issues!").arg(patch->getName()));
			break;
		default:
		case ProblemSeverity::None:
			ui->frame->clear();
			return;
	}

	auto &problems = patch->getProblems();
	QString problemOut;
	for (auto &problem: problems)
	{
		if(problem.m_severity == ProblemSeverity::Error)
		{
			problemOut += tr("Error: ");
		}
		else if(problem.m_severity == ProblemSeverity::Warning)
		{
			problemOut += tr("Warning: ");
		}
		problemOut += problem.m_description;
		problemOut += "\n";
	}
	ui->frame->setModDescription(problemOut);
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
	m_container->refreshContainer();
}

void VersionPage::on_removeBtn_clicked()
{
	if (ui->packageView->currentIndex().isValid())
	{
		// FIXME: use actual model, not reloading.
		if (!m_profile->remove(ui->packageView->currentIndex().row()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't remove file"));
		}
	}
	updateButtons();
	reloadMinecraftProfile();
	m_container->refreshContainer();
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
	auto list = GuiUtil::BrowseForFiles("jarmod", tr("Select jar mods"), tr("Minecraft.jar mods (*.zip *.jar)"), MMC->settings()->get("CentralModsDir").toString(), this->parentWidget());
	if(!list.empty())
	{
		m_profile->installJarMods(list);
	}
	updateButtons();
}

void VersionPage::on_jarBtn_clicked()
{
	auto jarPath = GuiUtil::BrowseForFile("jar", tr("Select jar"), tr("Minecraft.jar replacement (*.jar)"), MMC->settings()->get("CentralModsDir").toString(), this->parentWidget());
	if(!jarPath.isEmpty())
	{
		m_profile->installCustomJar(jarPath);
	}
	updateButtons();
}

void VersionPage::on_resetOrderBtn_clicked()
{
	try
	{
		m_profile->resetOrder();
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
		m_profile->move(currentRow(), MinecraftProfile::MoveUp);
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
		m_profile->move(currentRow(), MinecraftProfile::MoveDown);
	}
	catch (Exception &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
	updateButtons();
}

void VersionPage::on_changeVersionBtn_clicked()
{
	auto versionRow = currentRow();
	if(versionRow == -1)
	{
		return;
	}
	auto patch = m_profile->versionPatch(versionRow);
	auto name = patch->getName();
	auto list = patch->getVersionList();
	if(!list)
	{
		return;
	}
	auto uid = list->uid();
	VersionSelectDialog vselect(list.get(), tr("Change %1 version").arg(name), this);
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

	qDebug() << "Change" << uid << "to" << vselect.selectedVersion()->descriptor();
	if(uid == "net.minecraft")
	{
		if (!m_profile->isVanilla())
		{
			auto result = CustomMessageBox::selectable(
				this, tr("Are you sure?"),
				tr("This will remove any library/version customization you did previously. "
				"This includes things like Forge install and similar."),
				QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Abort,
				QMessageBox::Abort)->exec();

			if (result != QMessageBox::Ok)
				return;
			m_profile->revertToVanilla();
			reloadMinecraftProfile();
		}
	}
	m_inst->setComponentVersion(uid, vselect.selectedVersion()->descriptor());
	doUpdate();
	m_container->refreshContainer();
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
	m_container->refreshContainer();
	return ret;
}

void VersionPage::on_forgeBtn_clicked()
{
	auto vlist = ENV.metadataIndex()->get("net.minecraftforge");
	if(!vlist)
	{
		return;
	}
	VersionSelectDialog vselect(vlist.get(), tr("Select Forge version"), this);
	vselect.setExactFilter(BaseVersionList::ParentVersionRole, m_inst->getComponentVersion("net.minecraft"));
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") + m_inst->getComponentVersion("net.minecraft"));
	vselect.setEmptyErrorString(tr("Couldn't load or download the Forge version lists!"));
	if (vselect.exec() && vselect.selectedVersion())
	{
		auto vsn = vselect.selectedVersion();
		m_inst->setComponentVersion("net.minecraftforge", vsn->descriptor());
		m_profile->reload();
		// m_profile->installVersion();
		preselect(m_profile->rowCount(QModelIndex())-1);
		m_container->refreshContainer();
	}
}

void VersionPage::on_liteloaderBtn_clicked()
{
	auto vlist = ENV.metadataIndex()->get("com.mumfrey.liteloader");
	if(!vlist)
	{
		return;
	}
	VersionSelectDialog vselect(vlist.get(), tr("Select LiteLoader version"), this);
	vselect.setExactFilter(BaseVersionList::ParentVersionRole, m_inst->getComponentVersion("net.minecraft"));
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") + m_inst->getComponentVersion("net.minecraft"));
	vselect.setEmptyErrorString(tr("Couldn't load or download the LiteLoader version lists!"));
	if (vselect.exec() && vselect.selectedVersion())
	{
		auto vsn = vselect.selectedVersion();
		m_inst->setComponentVersion("com.mumfrey.liteloader", vsn->descriptor());
		m_profile->reload();
		// m_profile->installVersion(vselect.selectedVersion());
		preselect(m_profile->rowCount(QModelIndex())-1);
		m_container->refreshContainer();
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
	if(row >= m_profile->rowCount(QModelIndex()))
	{
		row = m_profile->rowCount(QModelIndex()) - 1;
	}
	if(row < 0)
	{
		return;
	}
	auto model_index = m_profile->index(row);
	ui->packageView->selectionModel()->select(model_index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
	updateButtons(row);
}

void VersionPage::updateButtons(int row)
{
	if(row == -1)
		row = currentRow();
	auto patch = m_profile->versionPatch(row);
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
		ui->editBtn->setEnabled(patch->isCustom());
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
	return m_profile->versionPatch(row);
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
	auto patch = m_profile->versionPatch(version);
	if(!patch->getVersionFile())
	{
		// TODO: wait for the update task to finish here...
		return;
	}
	if(!m_profile->customize(version))
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
	auto filename = version->getFilename();
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
	if(!m_profile->revertToBase(version))
	{
		// TODO: some error box here
	}
	updateButtons();
	preselect(currentIdx);
	m_container->refreshContainer();
}

#include "VersionPage.moc"

