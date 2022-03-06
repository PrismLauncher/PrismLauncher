#include "FlameModPage.h"
#include "ui_ModPage.h"

#include <QKeyEvent>

#include "Application.h"
#include "FlameModModel.h"
#include "InstanceImportTask.h"
#include "Json.h"
#include "ModDownloadTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

FlameModPage::FlameModPage(ModDownloadDialog* dialog, BaseInstance* instance) 
    : ModPage(dialog, instance, new FlameAPI())
{
    listModel = new FlameMod::ListModel(this);
    ui->packView->setModel(listModel);

    // index is used to set the sorting with the flame api
    ui->sortByBox->addItem(tr("Sort by Featured"));
    ui->sortByBox->addItem(tr("Sort by Popularity"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by Name"));
    ui->sortByBox->addItem(tr("Sort by Author"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems, 
    // so it's best not to connect them in the parent's contructor...
    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlameModPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlameModPage::onVersionSelectionChanged);
    connect(ui->modSelectionButton, &QPushButton::clicked, this, &FlameModPage::onModSelected);
}

bool FlameModPage::shouldDisplay() const { return true; }

void FlameModPage::onModVersionSucceed(ModPage* instance, QByteArray* response, QString addonId)
{
    if (addonId != current.addonId) {
        return;  // wrong request
    }
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Flame at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return;
    }
    QJsonArray arr = doc.array();
    try {
        FlameMod::loadIndexedPackVersions(current, arr, APPLICATION->network(), m_instance);
    } catch (const JSONValidationError& e) {
        qDebug() << *response;
        qWarning() << "Error while reading Flame mod version: " << e.cause();
    }
    auto packProfile = ((MinecraftInstance*)m_instance)->getPackProfile();
    QString mcVersion = packProfile->getComponentVersion("net.minecraft");
    QString loaderString = (packProfile->getComponentVersion("net.minecraftforge").isEmpty()) ? "fabric" : "forge";
    for (int i = 0; i < current.versions.size(); i++) {
        auto version = current.versions[i];
        if (!version.mcVersion.contains(mcVersion)) { continue; }
        ui->versionSelectionBox->addItem(version.version, QVariant(i));
    }
    if (ui->versionSelectionBox->count() == 0) { ui->versionSelectionBox->addItem(tr("No Valid Version found!"), QVariant(-1)); }

    ui->modSelectionButton->setText(tr("Cannot select invalid version :("));
    updateSelectionButton();
}
