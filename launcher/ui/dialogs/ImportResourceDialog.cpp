#include "ImportResourceDialog.h"
#include "ui_ImportResourceDialog.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <utility>

#include "Application.h"
#include "InstanceList.h"

#include "minecraft/PackProfile.h"
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
    connect(m_ui->searchEdit, &QLineEdit::textChanged, m_proxyModel, &InstanceProxyModel::setSearchTerm);
    connect(m_ui->button_show_all, &QPushButton::toggled, this, &ImportResourceDialog::showAllInstances);

    m_ui->label->setText(
        tr("Choose the instance you would like to import this %1 to.").arg(ModPlatform::ResourceTypeUtils::getName(m_resourceType)));
    m_ui->label_file_path->setText(tr("File: %1").arg(m_filePath));

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    setSelectedInstanceKey({});
    selectFirstInstance();
}

void ImportResourceDialog::setSelectedInstanceKey(QString key)
{
    selectedInstanceKey = std::move(key);
    // NOTE: the OK button is always clickable, so an empty key would be accepted otherwise
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!selectedInstanceKey.isEmpty());
}

void ImportResourceDialog::selectFirstInstance()
{
    auto* selectionModel = m_ui->instanceView->selectionModel();
    if (!selectionModel) {
        return;
    }
    if (selectionModel->hasSelection() && !selectedInstanceKey.isEmpty()) {
        return;
    }
    if (m_proxyModel->rowCount() == 0) {
        setSelectedInstanceKey({});
        return;
    }
    auto index = m_proxyModel->index(0, 0);
    selectionModel->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    selectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    m_ui->instanceView->scrollTo(index);
}

void ImportResourceDialog::activated(QModelIndex index)
{
    setSelectedInstanceKey(index.data(InstanceList::InstanceIDRole).toString());
    accept();
}

void ImportResourceDialog::selectionChanged(QItemSelection selected, QItemSelection /*deselected*/)
{
    if (selected.empty()) {
        setSelectedInstanceKey({});
        return;
    }

    setSelectedInstanceKey(selected.first().indexes().first().data(InstanceList::InstanceIDRole).toString());
}

ImportResourceDialog::~ImportResourceDialog()
{
    delete m_ui;
}
void ImportResourceDialog::sortBy(QStringList mcVersions, ModPlatform::ModLoaderTypes loader)
{
    auto* instances = APPLICATION->instances();
    for (int i = 0; i < instances->count(); ++i) {
        auto* inst = instances->at(i);
        if (auto res = inst->getPackProfile()->reload(Net::Mode::Offline); !res) {
            qWarning() << "Failed to reload components of" << inst->name() << ':' << res.error();
        }
    }
    m_mcVersions = mcVersions;
    m_loader = loader;
    m_proxyModel->sortBy(std::move(mcVersions), loader);
    m_proxyModel->invalidate();
    m_ui->button_show_all->setEnabled(m_loader != ModPlatform::ModLoaderType::None || !m_mcVersions.isEmpty());
    selectFirstInstance();
}

void ImportResourceDialog::showAllInstances(bool checked)
{
    if (checked) {
        m_proxyModel->sortBy({}, ModPlatform::ModLoaderType::None);
    } else {
        m_proxyModel->sortBy(m_mcVersions, m_loader);
    }
    m_proxyModel->invalidate();
    selectFirstInstance();
}
