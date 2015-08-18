//
// Created by robotbrain on 8/18/15.
//

#include "WorldListPage.h"
#include "ui_WorldListPage.h"
#include "minecraft/WorldList.h"
#include "dialogs/ModEditDialogCommon.h"
#include <QEvent>
#include <QKeyEvent>

WorldListPage::WorldListPage(BaseInstance *inst, std::shared_ptr<WorldList> worlds, QString id,
							 QString iconName, QString displayName, QString helpPage,
							 QWidget *parent)
	: QWidget(parent), ui(new Ui::WorldListPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	m_inst = inst;
	m_worlds = worlds;
	m_id = id;
	m_displayName = displayName;
	m_iconName = iconName;
	m_helpName = helpPage;
	ui->worldTreeView->setModel(m_worlds.get());
	ui->worldTreeView->installEventFilter(this);
	auto smodel = ui->worldTreeView->selectionModel();
	connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
			SLOT(modCurrent(QModelIndex, QModelIndex)));
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
