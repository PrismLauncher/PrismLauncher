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

#include "WorldListPage.h"
#include "ui_WorldListPage.h"
#include "minecraft/WorldList.h"
#include "dialogs/ModEditDialogCommon.h"
#include <QEvent>
#include <QKeyEvent>

WorldListPage::WorldListPage(BaseInstance *inst, std::shared_ptr<WorldList> worlds, QString id,
							 QString iconName, QString displayName, QString helpPage,
							 QWidget *parent)
	: QWidget(parent), ui(new Ui::WorldListPage), m_worlds(worlds), m_inst(inst), m_id(id), m_displayName(displayName), m_iconName(iconName), m_helpName(helpPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	ui->worldTreeView->setModel(m_worlds.get());
	ui->worldTreeView->installEventFilter(this);
}

void WorldListPage::opened()
{
	m_worlds->startWatching();
}

void WorldListPage::closed()
{
	m_worlds->stopWatching();
}

WorldListPage::~WorldListPage()
{
	m_worlds->stopWatching();
	delete ui;
}

bool WorldListPage::shouldDisplay() const
{
	if (m_inst)
		return !m_inst->isRunning();
	return true;
}

bool WorldListPage::worldListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmWorldBtn_clicked();
		return true;
	default:
		break;
	}
	return QWidget::eventFilter(ui->worldTreeView, keyEvent);
}

bool WorldListPage::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QWidget::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->worldTreeView)
		return worldListFilter(keyEvent);
	return QWidget::eventFilter(obj, ev);
}
void WorldListPage::on_rmWorldBtn_clicked()
{
	int first, last;
	auto list = ui->worldTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_worlds->stopWatching();
	m_worlds->deleteWorlds(first, last);
	m_worlds->startWatching();
}

void WorldListPage::on_viewFolderBtn_clicked()
{
	openDirInDefaultProgram(m_worlds->dir().absolutePath(), true);
}
