#include "ImportResourceDialog.h"
#include "ui_ImportResourceDialog.h"

#include <QFileDialog>
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
    connect(m_ui->button_show_all, &QPushButton::toggled, this, &ImportResourceDialog::showAllInstances);

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
}

void ImportResourceDialog::showAllInstances(bool checked)
{
    if (checked) {
        m_proxyModel->sortBy({}, ModPlatform::ModLoaderType::None);
    } else {
        m_proxyModel->sortBy(m_mcVersions, m_loader);
    }
    m_proxyModel->invalidate();
}
