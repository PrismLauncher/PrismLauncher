#include "ReviewMessageBox.h"
#include "ui_ReviewMessageBox.h"

ReviewMessageBox::ReviewMessageBox(QWidget* parent, QString const& title, QString const& icon)
    : QDialog(parent), ui(new Ui::ReviewMessageBox)
{
    ui->setupUi(this);

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

void ReviewMessageBox::appendMod(ModInformation&& info)
{
    auto itemTop = new QTreeWidgetItem(ui->modTreeWidget);
    itemTop->setCheckState(0, Qt::CheckState::Checked);
    itemTop->setText(0, info.name);

    auto filenameItem = new QTreeWidgetItem(itemTop);
    filenameItem->setText(0, tr("Filename: %1").arg(info.filename));

    itemTop->insertChildren(0, { filenameItem });

    ui->modTreeWidget->addTopLevelItem(itemTop);
}

auto ReviewMessageBox::deselectedMods() -> QStringList
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
