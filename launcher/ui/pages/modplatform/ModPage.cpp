#include "ModPage.h"
#include "ui_ModPage.h"

#include <QKeyEvent>

#include "ui/dialogs/ModDownloadDialog.h"

ModPage::ModPage(ModDownloadDialog* dialog, BaseInstance* instance, ModAPI* api)
    : QWidget(dialog), m_instance(instance), ui(new Ui::ModPage), dialog(dialog), api(api)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &ModPage::triggerSearch);
    ui->searchEdit->installEventFilter(this);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

}

ModPage::~ModPage()
{
    delete ui;
}

void ModPage::openedImpl()
{
    updateSelectionButton();
    triggerSearch();
}

bool ModPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ModPage::updateSelectionButton()
{
    if (!isOpened || selectedVersion < 0) {
        ui->modSelectionButton->setEnabled(false);
        return;
    }

    ui->modSelectionButton->setEnabled(true);
    auto& version = current.versions[selectedVersion];
    if (!dialog->isModSelected(current.name, version.fileName)) {
        ui->modSelectionButton->setText(tr("Select mod for download"));
    } else {
        ui->modSelectionButton->setText(tr("Deselect mod for download"));
    }
}

void ModPage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex());
}

void ModPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if (!first.isValid()) { return; }

    current = listModel->data(first, Qt::UserRole).value<ModPlatform::IndexedPack>();
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    
    if (!current.authors.empty()) {
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) {
            if (author.url.isEmpty()) { return author.name; }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : current.authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }
    text += "<br><br>";

    ui->packDescription->setHtml(text + current.description);

    if (!current.versionsLoaded) {
        qDebug() << QString("Loading %1 mod versions").arg(debugName());

        ui->modSelectionButton->setText(tr("Loading versions..."));
        ui->modSelectionButton->setEnabled(false);

        listModel->requestModVersions(current);
    } else {
        for (int i = 0; i < current.versions.size(); i++) {
            ui->versionSelectionBox->addItem(current.versions[i].version, QVariant(i));
        }
        if (ui->versionSelectionBox->count() == 0) { ui->versionSelectionBox->addItem(tr("No Valid Version found !"), QVariant(-1)); }

        updateSelectionButton();
    }
}

void ModPage::onVersionSelectionChanged(QString data)
{
    if (data.isNull() || data.isEmpty()) {
        selectedVersion = -1;
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toInt();
    updateSelectionButton();
}

void ModPage::onModSelected()
{
    auto& version = current.versions[selectedVersion];
    if (dialog->isModSelected(current.name, version.fileName)) {
        dialog->removeSelectedMod(current.name);
    } else {
        dialog->addSelectedMod(current.name, new ModDownloadTask(version.downloadUrl, version.fileName, dialog->mods));
    }

    updateSelectionButton();
}
