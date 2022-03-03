#include "ModrinthPage.h"
#include "ui_ModPage.h"

#include <QKeyEvent>

#include "Application.h"
#include "InstanceImportTask.h"
#include "Json.h"
#include "ModDownloadTask.h"
#include "ModrinthModel.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"

ModrinthPage::ModrinthPage(ModDownloadDialog* dialog, BaseInstance* instance)
    : ModPage(dialog, instance)
{
    listModel = new Modrinth::ListModel(this);
    ui->packView->setModel(listModel);

    // index is used to set the sorting with the modrinth api
    ui->sortByBox->addItem(tr("Sort by Relevence"));
    ui->sortByBox->addItem(tr("Sort by Downloads"));
    ui->sortByBox->addItem(tr("Sort by Follows"));
    ui->sortByBox->addItem(tr("Sort by last updated"));
    ui->sortByBox->addItem(tr("Sort by newest"));

    // sometimes Qt just ignores virtual slots and doesn't work as intended it seems, 
    // so it's best not to connect them in the parent's contructor...
    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthPage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &ModrinthPage::onVersionSelectionChanged);
    connect(ui->modSelectionButton, &QPushButton::clicked, this, &ModrinthPage::onModSelected);
}

bool ModrinthPage::shouldDisplay() const { return true; }

void ModrinthPage::onModVersionSucceed(ModPage* instance, QByteArray* response, QString addonId)
{
    if (addonId != current.addonId) { return; }
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset
                   << " reason: " << parse_error.errorString();
        qWarning() << *response;
        return;
    }
    QJsonArray arr = doc.array();
    try {
        Modrinth::loadIndexedPackVersions(current, arr, APPLICATION->network(), m_instance);
    } catch (const JSONValidationError& e) {
        qDebug() << *response;
        qWarning() << "Error while reading Modrinth mod version: " << e.cause();
    }
    auto packProfile = ((MinecraftInstance*)m_instance)->getPackProfile();
    QString mcVersion = packProfile->getComponentVersion("net.minecraft");
    QString loaderString = (packProfile->getComponentVersion("net.minecraftforge").isEmpty()) ? "fabric" : "forge";
    for (int i = 0; i < current.versions.size(); i++) {
        auto version = current.versions[i];
        if (!version.mcVersion.contains(mcVersion) || !version.loaders.contains(loaderString)) { continue; }
        ui->versionSelectionBox->addItem(version.version, QVariant(i));
    }
    if (ui->versionSelectionBox->count() == 0) { ui->versionSelectionBox->addItem(tr("No Valid Version found !"), QVariant(-1)); }

    ui->modSelectionButton->setText(tr("Cannot select invalid version :("));
    updateSelectionButton();
}
