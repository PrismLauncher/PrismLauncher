#include "ReviewMessageBox.h"
#include "ui_ReviewMessageBox.h"

#include "Application.h"

#include <QPushButton>

ReviewMessageBox::ReviewMessageBox(QWidget* parent, [[maybe_unused]] QString const& title, [[maybe_unused]] QString const& icon)
    : QDialog(parent), ui(new Ui::ReviewMessageBox)
{
    ui->setupUi(this);

    auto back_button = ui->buttonBox->button(QDialogButtonBox::Cancel);
    back_button->setText(tr("Back"));

    ui->toggleDepsButton->hide();
    ui->modTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->modTreeWidget->header()->setStretchLastSection(false);
    ui->modTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ReviewMessageBox::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ReviewMessageBox::reject);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

ReviewMessageBox::~ReviewMessageBox()
{
    delete ui;
}

auto ReviewMessageBox::create(QWidget* parent, QString&& title, QString&& icon) -> ReviewMessageBox*
{
    return new ReviewMessageBox(parent, title, icon);
}

void ReviewMessageBox::appendResource(ResourceInformation&& info)
{
    auto itemTop = new QTreeWidgetItem(ui->modTreeWidget);
    itemTop->setCheckState(0, info.enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    itemTop->setText(0, info.name);
    if (!info.enabled) {
        itemTop->setToolTip(0, tr("Mod was disabled as it may be already installed."));
    }

    auto filenameItem = new QTreeWidgetItem(itemTop);
    filenameItem->setText(0, tr("Filename: %1").arg(info.filename));

    auto childIndx = 0;
    itemTop->insertChildren(childIndx++, { filenameItem });

    if (!info.custom_file_path.isEmpty()) {
        auto customPathItem = new QTreeWidgetItem(itemTop);
        customPathItem->setText(0, tr("This download will be placed in: %1").arg(info.custom_file_path));

        itemTop->insertChildren(1, { customPathItem });

        itemTop->setIcon(1, QIcon(APPLICATION->getThemedIcon("status-yellow")));
        itemTop->setToolTip(
            childIndx++,
            tr("This file will be downloaded to a folder location different from the default, possibly due to its loader requiring it."));
    }

    auto providerItem = new QTreeWidgetItem(itemTop);
    providerItem->setText(0, tr("Provider: %1").arg(info.provider));

    itemTop->insertChildren(childIndx++, { providerItem });

    if (!info.required_by.isEmpty()) {
        auto requiredByItem = new QTreeWidgetItem(itemTop);
        if (info.required_by.length() == 1) {
            requiredByItem->setText(0, tr("Required by: %1").arg(info.required_by.back()));
        } else {
            requiredByItem->setText(0, tr("Required by:"));
            auto i = 0;
            for (auto req : info.required_by) {
                auto reqItem = new QTreeWidgetItem(requiredByItem);
                reqItem->setText(0, req);
                reqItem->insertChildren(i++, { reqItem });
            }
        }

        itemTop->insertChildren(childIndx++, { requiredByItem });
        ui->toggleDepsButton->show();
        m_deps << itemTop;
    }

    auto versionTypeItem = new QTreeWidgetItem(itemTop);
    versionTypeItem->setText(0, tr("Version Type: %1").arg(info.version_type));
    itemTop->insertChildren(childIndx++, { versionTypeItem });

    ui->modTreeWidget->addTopLevelItem(itemTop);
}

auto ReviewMessageBox::deselectedResources() -> QStringList
{
    QStringList list;

    auto* item = ui->modTreeWidget->topLevelItem(0);

    for (int i = 1; item != nullptr; ++i) {
        if (item->checkState(0) == Qt::CheckState::Unchecked) {
            list.append(item->text(0));
        }

        item = ui->modTreeWidget->topLevelItem(i);
    }

    return list;
}

void ReviewMessageBox::retranslateUi(QString resources_name)
{
    setWindowTitle(tr("Confirm %1 selection").arg(resources_name));

    ui->explainLabel->setText(tr("You're about to download the following %1:").arg(resources_name));
    ui->onlyCheckedLabel->setText(tr("Only %1 with a check will be downloaded!").arg(resources_name));
}
void ReviewMessageBox::on_toggleDepsButton_clicked()
{
    m_deps_checked = !m_deps_checked;
    auto state = m_deps_checked ? Qt::Checked : Qt::Unchecked;
    for (auto dep : m_deps)
        dep->setCheckState(0, state);
};