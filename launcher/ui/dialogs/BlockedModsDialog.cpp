#include "BlockedModsDialog.h"
#include "ui_BlockedModsDialog.h"
#include "qpushbutton.h"
#include <QDialogButtonBox>
#include <QDesktopServices>


BlockedModsDialog::BlockedModsDialog(QWidget *parent, const QString &title, const QString &text, const QString &body, const QList<QUrl> &urls) :
        QDialog(parent), ui(new Ui::BlockedModsDialog), urls(urls) {
    ui->setupUi(this);

    auto openAllButton = ui->buttonBox->addButton(tr("Open All"), QDialogButtonBox::ActionRole);
    connect(openAllButton, &QPushButton::clicked, this, &BlockedModsDialog::openAll);

    this->setWindowTitle(title);
    ui->label->setText(text);
    ui->textBrowser->setText(body);
}

BlockedModsDialog::~BlockedModsDialog() {
    delete ui;
}

void BlockedModsDialog::openAll() {
    for(auto &url : urls) {
        QDesktopServices::openUrl(url);
    }
}
