#include "ReviewMessageBox.h"
#include "ui_ReviewMessageBox.h"

ReviewMessageBox::ReviewMessageBox(QWidget* parent, QString const& title, QString const& icon)
    : QDialog(parent), ui(new Ui::ReviewMessageBox)
{
    ui->setupUi(this);
}

ReviewMessageBox::~ReviewMessageBox()
{
    delete ui;
}

auto ReviewMessageBox::create(QWidget* parent, QString&& title, QString&& icon) -> ReviewMessageBox*
{
    return new ReviewMessageBox(parent, title, icon);
}

void ReviewMessageBox::appendMod(const QString& name, const QString& filename)
{
    auto itemTop = new QTreeWidgetItem(ui->modTreeWidget);
    itemTop->setText(0, name);

    auto filenameItem = new QTreeWidgetItem(itemTop);
    filenameItem->setText(0, QString("Filename: %1").arg(filename));

    itemTop->insertChildren(0, { filenameItem });

    ui->modTreeWidget->addTopLevelItem(itemTop);
}
