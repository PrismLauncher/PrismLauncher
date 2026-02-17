#include "ImportResourceDialog.h"
#include "ui_ImportResourceDialog.h"

#include <QFileDialog>
#include <QPushButton>

#include "Application.h"
#include "InstanceList.h"

#include <InstanceList.h>
#include "modplatform/ResourceType.h"
#include "ui/instanceview/InstanceDelegate.h"
#include "ui/instanceview/InstanceProxyModel.h"

ImportResourceDialog::ImportResourceDialog(QString file_path, ModPlatform::ResourceType type, QWidget* parent)
    : QDialog(parent), ui(new Ui::ImportResourceDialog), m_resource_type(type), m_file_path(file_path)
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
    proxyModel->setSourceModel(APPLICATION->instances());
    proxyModel->sort(0);
    contentsWidget->setModel(proxyModel);

    connect(contentsWidget, &QAbstractItemView::doubleClicked, this, &ImportResourceDialog::activated);
    connect(contentsWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ImportResourceDialog::selectionChanged);

    ui->label->setText(
        tr("Choose the instance you would like to import this %1 to.").arg(ModPlatform::ResourceTypeUtils::getName(m_resource_type)));
    ui->label_file_path->setText(tr("File: %1").arg(m_file_path));

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

void ImportResourceDialog::activated(QModelIndex index)
{
    selectedInstanceKey = index.data(InstanceList::InstanceIDRole).toString();
    accept();
}

void ImportResourceDialog::selectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if (selected.empty())
        return;

    QString key = selected.first().indexes().first().data(InstanceList::InstanceIDRole).toString();
    if (!key.isEmpty()) {
        selectedInstanceKey = key;
    }
}

ImportResourceDialog::~ImportResourceDialog()
{
    delete ui;
}
