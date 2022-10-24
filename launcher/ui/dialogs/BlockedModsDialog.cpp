#include "BlockedModsDialog.h"
#include "ui_BlockedModsDialog.h"
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDesktopServices>

#include <QDebug>


BlockedModsDialog::BlockedModsDialog(QWidget *parent, const QString &title, const QString &text, const QList<BlockedMod> &mods) :
        QDialog(parent), ui(new Ui::BlockedModsDialog), mods(mods) {
    ui->setupUi(this);

    auto openAllButton = ui->buttonBox->addButton(tr("Open All"), QDialogButtonBox::ActionRole);
    connect(openAllButton, &QPushButton::clicked, this, &BlockedModsDialog::openAll);

    qDebug() << "Mods List: " << mods;

    this->setWindowTitle(title);
    ui->label->setText(text);
    ui->textBrowser->setText(body);
    update();
}

BlockedModsDialog::~BlockedModsDialog() {
    delete ui;
}

void BlockedModsDialog::openAll() {
    for(auto &mod : mods) {
        QDesktopServices::openUrl(mod.websiteUrl);
    }
}

void BlockedModsDialog::update() {
    QString text;
    QString span;

    for (auto &mod : mods) {
        if (mod.matched) {
            // &#x2714; -> html for HEAVY CHECK MARK : ✔
            span = QString("<span style=\"color:green\"> &#x2714; Found at %1 </span>").arg(mod.localPath);
        } else {
            // &#x2718; -> html for HEAVY BALLOT X : ✘
            span = QString("<span style=\"color:red\"> &#x2718; Not Found </span>");
        }
        text += QString("%1: <a href='%2'>%2</a> <p>Hash: %3 %4</p> <br/>").arg(mod.name, mod.websiteUrl, mod.hash, span);
    }

    ui->textBrowser->setText(text);
}


QDebug operator<<(QDebug debug, const BlockedMod &m) {
    QDebugStateSaver saver(debug);

    debug.nospace() << "{ name: " << m.name << ", websiteUrl: " << m.websiteUrl
        << ", hash: " << m.hash << ", matched: " << m.matched
        << ", localPath: " << m.localPath <<"}";

    return debug;
}