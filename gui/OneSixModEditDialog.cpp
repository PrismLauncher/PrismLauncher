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

#include "OneSixModEditDialog.h"
#include "ModEditDialogCommon.h"
#include "ui_OneSixModEditDialog.h"
#include <logic/ModList.h>
#include <pathutils.h>
#include <QFileDialog>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

OneSixModEditDialog::OneSixModEditDialog(OneSixInstance * inst, QWidget *parent):
	m_inst(inst),
	QDialog(parent),
	ui(new Ui::OneSixModEditDialog)
{
	ui->setupUi(this);
	//TODO: libraries!
	{
		// yeah... here be the real dragons.
	}
	// Loader mods
	{
		ensureFolderPathExists(m_inst->loaderModsDir());
		m_mods = m_inst->loaderModList();
		ui->loaderModTreeView->setModel(m_mods.data());
		ui->loaderModTreeView->installEventFilter( this );
		m_mods->startWatching();
	}
	// resource packs
	{
		ensureFolderPathExists(m_inst->resourcePacksDir());
		m_resourcepacks = m_inst->resourcePackList();
		ui->resPackTreeView->setModel(m_resourcepacks.data());
		ui->resPackTreeView->installEventFilter( this );
		m_resourcepacks->startWatching();
	}
}

OneSixModEditDialog::~OneSixModEditDialog()
{
	m_mods->stopWatching();
	m_resourcepacks->stopWatching();
	delete ui;
}

bool OneSixModEditDialog::loaderListFilter ( QKeyEvent* keyEvent )
{
	switch(keyEvent->key())
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
	return QDialog::eventFilter( ui->loaderModTreeView, keyEvent );
}

bool OneSixModEditDialog::resourcePackListFilter ( QKeyEvent* keyEvent )
{
	switch(keyEvent->key())
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
	return QDialog::eventFilter( ui->resPackTreeView, keyEvent );
}


bool OneSixModEditDialog::eventFilter ( QObject* obj, QEvent* ev )
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QDialog::eventFilter( obj, ev );
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
	if(obj == ui->loaderModTreeView)
		return loaderListFilter(keyEvent);
	if(obj == ui->resPackTreeView)
		return resourcePackListFilter(keyEvent);
	return QDialog::eventFilter( obj, ev );
}

void OneSixModEditDialog::on_buttonBox_rejected()
{
	close();
}

void OneSixModEditDialog::on_addModBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select Loader Mods");
	for(auto filename:fileNames)
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
	
	if(!lastfirst(list, first, last))
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
	QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select Resource Packs");
	for(auto filename:fileNames)
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
	
	if(!lastfirst(list, first, last))
		return;
	m_resourcepacks->stopWatching();
	m_resourcepacks->deleteMods(first, last);
	m_resourcepacks->startWatching();
}
void OneSixModEditDialog::on_viewResPackBtn_clicked()
{
	openDirInDefaultProgram(m_inst->resourcePacksDir(), true);
}

