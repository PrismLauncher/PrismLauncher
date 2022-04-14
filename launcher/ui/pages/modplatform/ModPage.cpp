#include "ModPage.h"
#include "ui_ModPage.h"

#include <QKeyEvent>
#include <memory>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

ModPage::ModPage(ModDownloadDialog* dialog, BaseInstance* instance, ModAPI* api)
    : QWidget(dialog)
    , m_instance(instance)
    , ui(new Ui::ModPage)
    , dialog(dialog)
    , filter_widget(static_cast<MinecraftInstance*>(instance)->getPackProfile()->getComponentVersion("net.minecraft"), this)
    , api(api)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &ModPage::triggerSearch);
    connect(ui->modFilterButton, &QPushButton::clicked, this, &ModPage::filterMods);
    ui->searchEdit->installEventFilter(this);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    ui->gridLayout_3->addWidget(&filter_widget, 0, 0, 1, ui->gridLayout_3->columnCount());

    filter_widget.setInstance(static_cast<MinecraftInstance*>(m_instance));
    m_filter = filter_widget.getFilter();
}

ModPage::~ModPage()
{
    delete ui;
}


/******** Qt things ********/

void ModPage::openedImpl()
{
    updateSelectionButton();
    triggerSearch();
}

auto ModPage::eventFilter(QObject* watched, QEvent* event) -> bool
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


/******** Callbacks to events in the UI (set up in the derived classes) ********/

void ModPage::filterMods()
{
    filter_widget.setHidden(!filter_widget.isHidden());
}

void ModPage::triggerSearch()
{
    auto changed = filter_widget.changed();
    m_filter = filter_widget.getFilter();
    
    if(changed){
        ui->packView->clearSelection();
        ui->packDescription->clear();
        ui->versionSelectionBox->clear();
        updateSelectionButton();
    }

    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex(), changed);
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
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) -> QString {
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
        if (ui->versionSelectionBox->count() == 0) { ui->versionSelectionBox->addItem(tr("No valid version found."), QVariant(-1)); }

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


/******** Make changes to the UI ********/

void ModPage::retranslate()
{
   ui->retranslateUi(this);
}

void ModPage::updateModVersions(int prev_count)
{
    auto packProfile = (dynamic_cast<MinecraftInstance*>(m_instance))->getPackProfile();

    QString mcVersion = packProfile->getComponentVersion("net.minecraft");

    QString loaderString = ModAPI::getModLoaderString(packProfile->getModLoader());

    for (int i = 0; i < current.versions.size(); i++) {
        auto version = current.versions[i];
        bool valid = false;
        for(auto& mcVer : m_filter->versions){
            //NOTE: Flame doesn't care about loaderString, so passing it changes nothing.
            if (validateVersion(version, mcVer.toString(), loaderString)) {
                valid = true;
                break;
            }
        }
        if(valid || m_filter->versions.size() == 0)
            ui->versionSelectionBox->addItem(version.version, QVariant(i));
    }
    if (ui->versionSelectionBox->count() == 0 && prev_count != 0) { 
        ui->versionSelectionBox->addItem(tr("No valid version found!"), QVariant(-1)); 
        ui->modSelectionButton->setText(tr("Cannot select invalid version :("));
    }

    updateSelectionButton();
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
