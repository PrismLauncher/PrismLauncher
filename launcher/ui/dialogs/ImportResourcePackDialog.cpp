#include "ImportResourcePackDialog.h"
#include "ui_ImportResourcePackDialog.h"

#include <QFileDialog>
#include <QPushButton>

#include "Application.h"
#include "InstanceList.h"

#include <InstanceList.h>
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceDelegate.h"

ImportResourcePackDialog::ImportResourcePackDialog(QWidget* parent) : QDialog(parent), ui(new Ui::ImportResourcePackDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);

    auto contentsWidget = ui->instanceView;
    contentsWidget->setViewMode(QListView::ListMode);
    contentsWidget->setFlow(QListView::LeftToRight);
    contentsWidget->setIconSize(QSize(48, 48));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setResizeMode(QListView::Adjust);
    contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    contentsWidget->setSpacing(5);
    contentsWidget->setWordWrap(true);
    contentsWidget->setWrapping(true);
    // NOTE: We can't have uniform sizes because the text may wrap if it's too long. If we set this, it will cut off the wrapped text.
    contentsWidget->setUniformItemSizes(false);
    contentsWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    contentsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentsWidget->setItemDelegate(new ListViewDelegate());

    proxyModel = new InstanceProxyModel(this);
    proxyModel->setSourceModel(APPLICATION->instances().get());
    proxyModel->sort(0);
    contentsWidget->setModel(proxyModel);

    connect(contentsWidget, SIGNAL(doubleClicked(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(contentsWidget->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            SLOT(selectionChanged(QItemSelection, QItemSelection)));
}

void ImportResourcePackDialog::activated(QModelIndex index)
{
    selectedInstanceKey = index.data(InstanceList::InstanceIDRole).toString();
    accept();
}

void ImportResourcePackDialog::selectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if (selected.empty())
        return;

    QString key = selected.first().indexes().first().data(InstanceList::InstanceIDRole).toString();
    if (!key.isEmpty()) {
        selectedInstanceKey = key;
    }
}

ImportResourcePackDialog::~ImportResourcePackDialog()
{
    delete ui;
}
