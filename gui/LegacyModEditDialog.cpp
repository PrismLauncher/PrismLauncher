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

#include "LegacyModEditDialog.h"
#include "ui_LegacyModEditDialog.h"
#include <logic/ModList.h>
#include <pathutils.h>
#include <QFileDialog>

LegacyModEditDialog::LegacyModEditDialog( LegacyInstance* inst, QWidget* parent ) :
	m_inst(inst),
	QDialog(parent),
	ui(new Ui::LegacyModEditDialog)
{
	ui->setupUi(this);
	ensurePathExists(m_inst->coreModsDir());
	ensurePathExists(m_inst->mlModsDir());
	ensurePathExists(m_inst->jarModsDir());
	
	m_mods = m_inst->loaderModList();
	m_coremods = m_inst->coreModList();
	m_jarmods = m_inst->jarModList();
	/*
	m_mods->startWatching();
	m_coremods->startWatching();
	m_jarmods->startWatching();
	*/
	
	ui->jarModsTreeView->setModel(m_jarmods.data());
	ui->coreModsTreeView->setModel(m_coremods.data());
	ui->mlModTreeView->setModel(m_mods.data());
}

LegacyModEditDialog::~LegacyModEditDialog()
{
	delete ui;
}

void LegacyModEditDialog::on_addCoreBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select Core Mods");
	for(auto filename:fileNames)
	{
		m_coremods->installMod(QFileInfo(filename));
	}
}
void LegacyModEditDialog::on_addForgeBtn_clicked()
{

}
void LegacyModEditDialog::on_addJarBtn_clicked()
{

}
void LegacyModEditDialog::on_addModBtn_clicked()
{

}
void LegacyModEditDialog::on_addTexPackBtn_clicked()
{

}
void LegacyModEditDialog::on_moveJarDownBtn_clicked()
{

}
void LegacyModEditDialog::on_moveJarUpBtn_clicked()
{

}
void LegacyModEditDialog::on_rmCoreBtn_clicked()
{
	auto sm = ui->coreModsTreeView->selectionModel();
	auto selection = sm->selectedRows();
	if(!selection.size())
		return;
	m_coremods->deleteMod(selection[0].row());
}
void LegacyModEditDialog::on_rmJarBtn_clicked()
{
	auto sm = ui->jarModsTreeView->selectionModel();
	auto selection = sm->selectedRows();
	if(!selection.size())
		return;
	m_jarmods->deleteMod(selection[0].row());
}
void LegacyModEditDialog::on_rmModBtn_clicked()
{
	auto sm = ui->mlModTreeView->selectionModel();
	auto selection = sm->selectedRows();
	if(!selection.size())
		return;
	m_mods->deleteMod(selection[0].row());
}
void LegacyModEditDialog::on_rmTexPackBtn_clicked()
{

}
void LegacyModEditDialog::on_viewCoreBtn_clicked()
{
	openDirInDefaultProgram(m_inst->coreModsDir(), true);
}
void LegacyModEditDialog::on_viewModBtn_clicked()
{
	openDirInDefaultProgram(m_inst->mlModsDir(), true);
}
void LegacyModEditDialog::on_viewTexPackBtn_clicked()
{
	//openDirInDefaultProgram(m_inst->mlModsDir(), true);
}


void LegacyModEditDialog::on_buttonBox_rejected()
{
	close();
}