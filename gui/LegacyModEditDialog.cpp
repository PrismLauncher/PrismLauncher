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
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

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
	
	ui->jarModsTreeView->setModel(m_jarmods.data());
	ui->coreModsTreeView->setModel(m_coremods.data());
	ui->mlModTreeView->setModel(m_mods.data());
	
	ui->jarModsTreeView->installEventFilter( this );
	ui->coreModsTreeView->installEventFilter( this );
	ui->mlModTreeView->installEventFilter( this );
}

LegacyModEditDialog::~LegacyModEditDialog()
{
	delete ui;
}

bool LegacyModEditDialog::coreListFilter ( QKeyEvent* keyEvent )
{
	switch(keyEvent->key())
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
	return QDialog::eventFilter( ui->coreModsTreeView, keyEvent );
}

bool LegacyModEditDialog::jarListFilter ( QKeyEvent* keyEvent )
{
	switch(keyEvent->key())
	{
		case Qt::Key_Up:
		{
			if(keyEvent->modifiers() & Qt::ControlModifier)
			{
				on_moveJarUpBtn_clicked();
				return true;
			}
			break;
		}
		case Qt::Key_Down:
		{
			if(keyEvent->modifiers() & Qt::ControlModifier)
			{
				on_moveJarDownBtn_clicked();
				return true;
			}
			break;
		}
		case Qt::Key_Delete:
			on_rmJarBtn_clicked();
			return true;
		case Qt::Key_Plus:
			on_addJarBtn_clicked();
			return true;
		default:
			break;
	}
	return QDialog::eventFilter( ui->jarModsTreeView, keyEvent );
}

bool LegacyModEditDialog::loaderListFilter ( QKeyEvent* keyEvent )
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
	return QDialog::eventFilter( ui->mlModTreeView, keyEvent );
}


bool LegacyModEditDialog::eventFilter ( QObject* obj, QEvent* ev )
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QDialog::eventFilter( obj, ev );
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
	if(obj == ui->jarModsTreeView)
		return jarListFilter(keyEvent);
	if(obj == ui->coreModsTreeView)
		return coreListFilter(keyEvent);
	if(obj == ui->mlModTreeView)
		return loaderListFilter(keyEvent);
	return QDialog::eventFilter( obj, ev );
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
	QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select Jar Mods");
	for(auto filename:fileNames)
	{
		m_jarmods->installMod(QFileInfo(filename));
	}
}
void LegacyModEditDialog::on_addModBtn_clicked()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select Loader Mods");
	for(auto filename:fileNames)
	{
		m_mods->installMod(QFileInfo(filename));
	}
}
void LegacyModEditDialog::on_addTexPackBtn_clicked()
{

}

bool lastfirst (QModelIndexList & list, int & first, int & last)
{
	if(!list.size())
		return false;
	first = last = list[0].row();
	for(auto item: list)
	{
		int row = item.row();
		if(row < first)
			first = row;
		if(row > last)
			last = row;
	}
	return true;
}

void LegacyModEditDialog::on_moveJarDownBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();
	
	if(!lastfirst(list, first, last))
		return;

	m_jarmods->moveModsDown(first, last);
}
void LegacyModEditDialog::on_moveJarUpBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();
	
	if(!lastfirst(list, first, last))
		return;
	m_jarmods->moveModsUp(first, last);
}
void LegacyModEditDialog::on_rmCoreBtn_clicked()
{
	int first, last;
	auto list = ui->coreModsTreeView->selectionModel()->selectedRows();
	
	if(!lastfirst(list, first, last))
		return;
	m_coremods->deleteMods(first, last);
}
void LegacyModEditDialog::on_rmJarBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();
	
	if(!lastfirst(list, first, last))
		return;
	m_jarmods->deleteMods(first, last);
}
void LegacyModEditDialog::on_rmModBtn_clicked()
{
	int first, last;
	auto list = ui->mlModTreeView->selectionModel()->selectedRows();
	
	if(!lastfirst(list, first, last))
		return;
	m_mods->deleteMods(first, last);
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