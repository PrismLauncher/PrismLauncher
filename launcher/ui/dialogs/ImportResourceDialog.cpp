#include "ImportResourceDialog.h"
#include "ui_ImportResourceDialog.h"

#include <QFileDialog>
#include <QPushButton>
#include <utility>

#include "Application.h"
#include "InstanceList.h"

#include "modplatform/ResourceType.h"
#include "ui/instanceview/InstanceDelegate.h"
#include "ui/instanceview/InstanceProxyModel.h"

ImportResourceDialog::ImportResourceDialog(QString filePath, ModPlatform::ResourceType type, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::ImportResourceDialog)
    , m_resourceType(type)
    , m_filePath(std::move(filePath))
    , m_proxyModel(new InstanceProxyModel(this))
{
    m_ui->setupUi(this);
    setWindowModality(Qt::WindowModal);

    auto* contentsWidget = m_ui->instanceView;
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

    m_proxyModel->setSourceModel(APPLICATION->instances());
    m_proxyModel->sort(0);
    contentsWidget->setModel(m_proxyModel);

    connect(contentsWidget, &QAbstractItemView::doubleClicked, this, &ImportResourceDialog::activated);
    connect(contentsWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ImportResourceDialog::selectionChanged);

    m_ui->label->setText(
        tr("Choose the instance you would like to import this %1 to.").arg(ModPlatform::ResourceTypeUtils::getName(m_resourceType)));
    m_ui->label_file_path->setText(tr("File: %1").arg(m_filePath));

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

void ImportResourceDialog::activated(QModelIndex index)
{
    selectedInstanceKey = index.data(InstanceList::InstanceIDRole).toString();
    accept();
}

void ImportResourceDialog::selectionChanged(QItemSelection selected, QItemSelection /*deselected*/)
{
    if (selected.empty()) {
        return;
    }

    QString key = selected.first().indexes().first().data(InstanceList::InstanceIDRole).toString();
    if (!key.isEmpty()) {
        selectedInstanceKey = key;
    }
}

ImportResourceDialog::~ImportResourceDialog()
{
    delete m_ui;
}
