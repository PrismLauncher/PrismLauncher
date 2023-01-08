#include "ReviewMessageBox.h"
#include "ui_ReviewMessageBox.h"

#include "Application.h"

#include <QPushButton>

ReviewMessageBox::ReviewMessageBox(QWidget* parent, QString const& title, QString const& icon)
    : QDialog(parent), ui(new Ui::ReviewMessageBox)
{
    ui->setupUi(this);

    auto back_button = ui->buttonBox->button(QDialogButtonBox::Cancel);
    back_button->setText(tr("Back"));

    ui->modTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->modTreeWidget->header()->setStretchLastSection(false);
    ui->modTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ReviewMessageBox::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ReviewMessageBox::reject);
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
    itemTop->setCheckState(0, Qt::CheckState::Checked);
    itemTop->setText(0, info.name);

    auto filenameItem = new QTreeWidgetItem(itemTop);
    filenameItem->setText(0, tr("Filename: %1").arg(info.filename));

    itemTop->insertChildren(0, { filenameItem });

    if (!info.custom_file_path.isEmpty()) {
        auto customPathItem = new QTreeWidgetItem(itemTop);
        customPathItem->setText(0, tr("This download will be placed in: %1").arg(info.custom_file_path));

        itemTop->insertChildren(1, { customPathItem });

        itemTop->setIcon(1, QIcon(APPLICATION->getThemedIcon("status-yellow")));
        itemTop->setToolTip(1, tr("This file will be downloaded to a folder location different from the default, possibly due to its loader requiring it."));
    }

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
