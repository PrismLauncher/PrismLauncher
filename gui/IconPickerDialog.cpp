#include "IconPickerDialog.h"
#include "instancedelegate.h"
#include "ui_IconPickerDialog.h"
#include "logic/IconListModel.h"

IconPickerDialog::IconPickerDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::IconPickerDialog)
{
	ui->setupUi(this);
	auto contentsWidget = ui->iconView;
	contentsWidget->setViewMode(QListView::IconMode);
	contentsWidget->setFlow(QListView::LeftToRight);
	contentsWidget->setIconSize(QSize(48, 48));
	contentsWidget->setMovement(QListView::Static);
	contentsWidget->setResizeMode(QListView::Adjust);
	contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	contentsWidget->setSpacing(5);
	contentsWidget->setWordWrap(false);
	contentsWidget->setWrapping(true);
	contentsWidget->setUniformItemSizes(true);
	contentsWidget->setTextElideMode(Qt::ElideRight);
	contentsWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	contentsWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	contentsWidget->setItemDelegate(new ListViewDelegate());
	
	IconList * list = IconList::instance();
	contentsWidget->setModel(list);
	
	connect
	(
		contentsWidget,
		SIGNAL(doubleClicked(QModelIndex)),
		SLOT(activated(QModelIndex))
	);
	
	connect
	(
		contentsWidget->selectionModel(),
		SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
		SLOT(selectionChanged(QItemSelection,QItemSelection))
	);
}

void IconPickerDialog::activated ( QModelIndex index )
{
	selectedIconKey = index.data(Qt::UserRole).toString();
	accept();
}


void IconPickerDialog::selectionChanged ( QItemSelection selected, QItemSelection deselected )
{
	if(selected.empty())
		return;
	
	QString key = selected.first().indexes().first().data(Qt::UserRole).toString();
	if(!key.isEmpty())
		selectedIconKey = key;
}

int IconPickerDialog::exec ( QString selection )
{
	IconList * list = IconList::instance();
	auto contentsWidget = ui->iconView;
	selectedIconKey = selection;
	
	
	int index_nr = list->getIconIndex(selection);
	auto model_index = list->index(index_nr);
	contentsWidget->selectionModel()->select(model_index, QItemSelectionModel::Current | QItemSelectionModel::Select);
	
	QMetaObject::invokeMethod(this, "delayed_scroll", Qt::QueuedConnection, Q_ARG(QModelIndex,model_index));
	return QDialog::exec();
}

void IconPickerDialog::delayed_scroll ( QModelIndex model_index )
{
	auto contentsWidget = ui->iconView;
	contentsWidget->scrollTo(model_index);
}


IconPickerDialog::~IconPickerDialog()
{
	delete ui;
}
