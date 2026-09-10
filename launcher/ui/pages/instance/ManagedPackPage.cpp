// SPDX-FileCopyrightText: 2022 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ManagedPackPage.h"
#include <QDesktopServices>
#include <QLineEdit>
#include <QUrl>
#include <QUrlQuery>
#include "modplatform/ModIndex.h"
#include "ui_ManagedPackPage.h"

#include <QFileDialog>
#include <memory>

#include "Application.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "InstanceTask.h"
#include "Markdown.h"
#include "StringUtils.h"
#include "config/InstanceConfig.h"

#include "ui/InstanceWindow.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

ManagedPackPage* ManagedPackPage::createPage(BaseInstance* inst, QWidget* parent)
{
    const auto& managedPack = inst->config()->managedPack;
    if (managedPack.has_value()) {
        if (managedPack->type == "modrinth") {
            return new ModrinthManagedPackPage(inst, nullptr, parent);
        }
        if (managedPack->type == "flame" && ((APPLICATION->capabilities() & Application::SupportsFlame) != 0U)) {
            return new FlameManagedPackPage(inst, nullptr, parent);
        }
    }

    return new GenericManagedPackPage(inst, nullptr, parent);
}

ManagedPackPage::ManagedPackPage(BaseInstance* inst, InstanceWindow* instanceWindow, QWidget* parent)
    : QWidget(parent), m_instanceWindow(instanceWindow), ui(new Ui::ManagedPackPage), m_inst(inst)
{
    Q_ASSERT(inst);

    ui->setupUi(this);

    ui->versionsComboBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionsComboBox->view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    ui->reloadButton->setVisible(false);
    connect(ui->reloadButton, &QPushButton::clicked, this, [this](bool) {
        ui->reloadButton->setVisible(false);

        m_loaded = false;
        // Pretend we're opening the page again
        openedImpl();
    });

    connect(ui->changelogTextBrowser, &QTextBrowser::anchorClicked, this, [](const QUrl& url) {
        if (url.scheme().isEmpty()) {
            auto querry =
                QUrlQuery(url.query()).queryItemValue("remoteUrl", QUrl::FullyDecoded);  // curseforge workaround for linkout?remoteUrl=
            auto decoded = QUrl::fromPercentEncoding(querry.toUtf8());
            auto newUrl = QUrl(decoded);
            if (newUrl.isValid() && (newUrl.scheme() == "http" || newUrl.scheme() == "https")) {
                QDesktopServices ::openUrl(newUrl);
            }
            return;
        }
        QDesktopServices::openUrl(url);
    });

    connect(ui->urlLine, &QLineEdit::textChanged, this, [this](const QString& text) {
        auto& managedPack = m_inst->config().update().managedPack;
        Q_ASSERT(managedPack.has_value());

        managedPack->url = text.trimmed();
    });
}

ManagedPackPage::~ManagedPackPage()
{
    delete ui;
}

void ManagedPackPage::openedImpl()
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    if (managedPack->id.isEmpty()) {
        ui->packVersion->hide();
        ui->packVersionLabel->hide();
        ui->packOrigin->hide();
        ui->packOriginLabel->hide();
        ui->versionsComboBox->hide();
        ui->updateToVersionLabel->setText(tr("URL:"));
        ui->updateButton->setText(tr("Update Pack"));
        ui->updateButton->setDisabled(false);
        ui->urlLine->setText(managedPack->url);

        ui->packName->setText(m_inst->name());
        ui->changelogTextBrowser->setText(tr("This is a local modpack.\n"
                                             "This can be updated either using a file in %1 format or an URL.\n"
                                             "Do not use a different format than the one mentioned as it may break the instance.\n"
                                             "Make sure you also trust the URL.\n")
                                              .arg(displayName()));
        return;
    }
    ui->urlLine->hide();
    ui->packName->setText(managedPack->name);
    ui->packVersion->setText(managedPack->versionName);
    ui->packOrigin->setText(tr("Website: <a href=%1>%2</a>    |    Pack ID: %3    |    Version ID: %4")
                                .arg(url(), displayName(), managedPack->id, managedPack->versionId));

    parseManagedPack();
}

QString ManagedPackPage::displayName() const
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    auto type = managedPack->type;
    if (type.isEmpty()) {
        return {};
    }
    if (type == "flame") {
        type = "CurseForge";
    }
    return type.replace(0, 1, type[0].toUpper());
}

QIcon ManagedPackPage::icon() const
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());
    return QIcon::fromTheme(managedPack->type);
}

QString ManagedPackPage::helpPage() const
{
    return {};
}

void ManagedPackPage::retranslate()
{
    ui->retranslateUi(this);
}

bool ManagedPackPage::shouldDisplay() const
{
    return m_inst->config()->managedPack.has_value();
}

bool ManagedPackPage::runUpdateTask(InstanceTask* task)
{
    Q_ASSERT(task);

    const unique_qobject_ptr<Task> wrappedTask(APPLICATION->instances()->wrapInstanceTask(task));

    connect(wrappedTask.get(), &Task::failed, this,
            [this](const QString& reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(wrappedTask.get(), &Task::succeeded, this, [this, task]() {
        QStringList warnings = task->warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
        }
    });

    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(wrappedTask.get());

    return wrappedTask->wasSuccessful();
}

void ManagedPackPage::suggestVersion()
{
    ui->updateButton->setText(tr("Update Pack"));
    ui->updateButton->setDisabled(false);
}

void ManagedPackPage::setFailState()
{
    qDebug() << "Setting fail state!";

    // We block signals here so that suggestVersion() doesn't get called, causing an assertion fail.
    ui->versionsComboBox->blockSignals(true);
    ui->versionsComboBox->clear();
    ui->versionsComboBox->addItem(tr("Failed to search for available versions."), {});
    ui->versionsComboBox->blockSignals(false);

    ui->changelogTextBrowser->setText(tr("Failed to request changelog data for this modpack."));

    ui->updateButton->setText(tr("Cannot update!"));
    ui->updateButton->setDisabled(true);

    ui->reloadButton->setVisible(true);
}

ModrinthManagedPackPage::ModrinthManagedPackPage(BaseInstance* inst, InstanceWindow* instanceWindow, QWidget* parent)
    : ManagedPackPage(inst, instanceWindow, parent)
{
    Q_ASSERT(inst->config()->managedPack.has_value());
    connect(ui->versionsComboBox, &QComboBox::currentIndexChanged, this, &ModrinthManagedPackPage::suggestVersion);
    connect(ui->updateButton, &QPushButton::clicked, this, &ModrinthManagedPackPage::update);
    connect(ui->updateFromFileButton, &QPushButton::clicked, this, &ModrinthManagedPackPage::updateFromFile);
}

// MODRINTH
void ModrinthManagedPackPage::parseManagedPack()
{
    qDebug() << "Parsing Modrinth pack";

    // No need for the extra work because we already have everything we need.
    if (m_loaded) {
        return;
    }

    if (m_fetchJob && m_fetchJob->isRunning()) {
        m_fetchJob->abort();
    }

    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion>> callbacks{};
    m_pack = { .addonId = managedPack->id };

    // Use default if no callbacks are set
    callbacks.onSucceed = [this](auto& doc) {
        m_pack.versions = doc;
        m_pack.versionsLoaded = true;

        // We block signals here so that suggestVersion() doesn't get called, causing an assertion fail.
        ui->versionsComboBox->blockSignals(true);
        ui->versionsComboBox->clear();
        ui->versionsComboBox->blockSignals(false);

        const auto& managedPack = m_inst->config()->managedPack;
        Q_ASSERT(managedPack.has_value());

        for (const auto& version : m_pack.versions) {
            QString name = version.getVersionDisplayString();

            // NOTE: the id from version isn't the same id in the modpack format spec...
            // e.g. HexMC's 4.4.0 has versionId 4.0.0 in the modpack index..............
            if (version.version == managedPack->versionName) {
                name = tr("%1 (Current)").arg(name);
            }

            ui->versionsComboBox->addItem(name, version.fileId);
        }

        suggestVersion();

        m_loaded = true;
    };
    callbacks.onFail = [this](const QString& /*reason*/, int) { setFailState(); };
    callbacks.onAbort = [this]() { setFailState(); };
    m_fetchJob = ModrinthAPI::get().getProjectVersions({ .pack = std::make_shared<ModPlatform::IndexedPack>(m_pack),
                                                         .mcVersions = {},
                                                         .loaders = {},
                                                         .resourceType = ModPlatform::ResourceType::Modpack,
                                                         .includeChangelog = true },
                                                       callbacks);

    ui->changelogTextBrowser->setText(tr("Fetching changelogs..."));

    m_fetchJob->start();
}

QString ModrinthManagedPackPage::url() const
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    return "https://modrinth.com/mod/" + managedPack->id;
}

void ModrinthManagedPackPage::suggestVersion()
{
    auto index = ui->versionsComboBox->currentIndex();
    if (m_pack.versions.length() == 0) {
        setFailState();
        return;
    }
    auto version = m_pack.versions.at(index);

    ui->changelogTextBrowser->setHtml(StringUtils::htmlListPatch(markdownToHTML(version.changelog.toUtf8())));

    ManagedPackPage::suggestVersion();
}

/// @brief Called when the update task has completed.
/// Internally handles the closing of the instance window if the update was successful and shows a message box.
/// @param did_succeed Whether the update task was successful.
void ManagedPackPage::onUpdateTaskCompleted(bool didSucceed) const
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    // Close the window if the update was successful
    if (didSucceed) {
        if (m_instanceWindow != nullptr) {
            m_instanceWindow->close();
        }

        CustomMessageBox::selectable(nullptr, tr("Update Successful"),
                                     tr("The instance updated to pack version %1 successfully.").arg(managedPack->versionName),
                                     QMessageBox::Information)
            ->show();
    } else {
        CustomMessageBox::selectable(
            nullptr, tr("Update Failed"),
            tr("The instance failed to update to pack version %1. Please check launcher logs for more information.")
                .arg(managedPack->versionName),
            QMessageBox::Critical)
            ->show();
    }
}

void ModrinthManagedPackPage::update()
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    auto customURL = managedPack->url.trimmed();
    if (managedPack->id.isEmpty() && !customURL.isEmpty()) {
        updatePack(customURL, false);
        return;
    }
    auto index = ui->versionsComboBox->currentIndex();
    if (m_pack.versions.length() == 0) {
        setFailState();
        return;
    }
    auto version = m_pack.versions.at(index);

    updatePack(version.downloadUrl, true, version.fileId.toString(), version.version);
}

void ModrinthManagedPackPage::updateFromFile()
{
    auto output = QFileDialog::getOpenFileUrl(this, tr("Choose update file"), QDir::homePath(), tr("Modrinth pack") + " (*.mrpack *.zip)");
    if (output.isEmpty()) {
        return;
    }

    updatePack(output, false);
}

// FLAME
FlameManagedPackPage::FlameManagedPackPage(BaseInstance* inst, InstanceWindow* instanceWindow, QWidget* parent)
    : ManagedPackPage(inst, instanceWindow, parent)
{
    Q_ASSERT(inst->config()->managedPack.has_value());
    connect(ui->versionsComboBox, &QComboBox::currentIndexChanged, this, &FlameManagedPackPage::suggestVersion);
    connect(ui->updateButton, &QPushButton::clicked, this, &FlameManagedPackPage::update);
    connect(ui->updateFromFileButton, &QPushButton::clicked, this, &FlameManagedPackPage::updateFromFile);
}

void FlameManagedPackPage::parseManagedPack()
{
    qDebug() << "Parsing Flame pack";

    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    // We need to tell the user to redownload the pack, since we didn't save the required info previously
    if (managedPack->id.isEmpty()) {
        setFailState();
        QString message =
            tr("<h1>Hey there!</h1>"
               "<h4>"
               "It seems like your Pack ID is null. This is because of a bug in older versions of the launcher.<br/>"
               "Unfortunately, we can't do the proper API requests without this information.<br/>"
               "<br/>"
               "So, in order for this feature to work, you will need to re-download the modpack from the built-in downloader.<br/>"
               "<br/>"
               "Don't worry though, it will ask you to update this instance instead, so you'll not lose this instance!"
               "</h4>");

        ui->changelogTextBrowser->setHtml(StringUtils::htmlListPatch(message));
        return;
    }

    // No need for the extra work because we already have everything we need.
    if (m_loaded) {
        return;
    }

    if (m_fetchJob && m_fetchJob->isRunning()) {
        m_fetchJob->abort();
    }

    m_pack = { .addonId = managedPack->id };

    ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion>> callbacks{};

    // Use default if no callbacks are set
    callbacks.onSucceed = [this](auto& doc) {
        m_pack.versions = doc;
        m_pack.versionsLoaded = true;

        // We block signals here so that suggestVersion() doesn't get called, causing an assertion fail.
        ui->versionsComboBox->blockSignals(true);
        ui->versionsComboBox->clear();
        ui->versionsComboBox->blockSignals(false);

        const auto& managedPack = m_inst->config()->managedPack;
        Q_ASSERT(managedPack.has_value());

        for (const auto& version : m_pack.versions) {
            QString name = version.getVersionDisplayString();

            if (version.fileId == managedPack->versionId) {
                name = tr("%1 (Current)").arg(name);
            }

            ui->versionsComboBox->addItem(name, QVariant(version.fileId));
        }

        suggestVersion();

        m_loaded = true;
    };
    callbacks.onFail = [this](const QString& /*reason*/, int) { setFailState(); };
    callbacks.onAbort = [this]() { setFailState(); };
    m_fetchJob = FlameAPI::get().getProjectVersions({ .pack = std::make_shared<ModPlatform::IndexedPack>(m_pack),
                                                      .mcVersions = {},
                                                      .loaders = {},
                                                      .resourceType = ModPlatform::ResourceType::Modpack,
                                                      .includeChangelog = true },
                                                    callbacks);

    m_fetchJob->start();
}

QString FlameManagedPackPage::url() const
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    // FIXME: We should display the websiteUrl field, but this requires doing the API request first :(
    return "https://www.curseforge.com/projects/" + managedPack->id;
}

void FlameManagedPackPage::suggestVersion()
{
    auto index = ui->versionsComboBox->currentIndex();
    if (m_pack.versions.length() == 0) {
        setFailState();
        return;
    }
    auto version = m_pack.versions.at(index);

    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    ui->changelogTextBrowser->setHtml(
        StringUtils::htmlListPatch(FlameAPI::getModFileChangelog(managedPack->id.toInt(), version.fileId.toInt())));

    ManagedPackPage::suggestVersion();
}

void FlameManagedPackPage::update()
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    auto customURL = managedPack->url.trimmed();
    if (managedPack->id.isEmpty() && !customURL.isEmpty()) {
        updatePack(customURL, false);
        return;
    }
    auto index = ui->versionsComboBox->currentIndex();
    if (m_pack.versions.length() == 0) {
        setFailState();
        return;
    }
    auto version = m_pack.versions.at(index);

    updatePack(version.downloadUrl, true, version.fileId.toString());
}

void FlameManagedPackPage::updateFromFile()
{
    auto output = QFileDialog::getOpenFileUrl(this, tr("Choose update file"), QDir::homePath(), tr("CurseForge pack") + " (*.zip)");
    if (output.isEmpty()) {
        return;
    }

    updatePack(output, false);
}

void ManagedPackPage::updatePack(const QUrl& url, bool trusted, const QString& versionID, const QString& versionName)
{
    const auto& managedPack = m_inst->config()->managedPack;
    Q_ASSERT(managedPack.has_value());

    QMap<QString, QString> extraInfo;
    // NOTE: Don't use 'm_pack.id' here, since we didn't completely parse all the metadata for the pack, including this field.
    extraInfo.insert("pack_id", managedPack->id);
    extraInfo.insert("pack_version_id", versionID);
    extraInfo.insert("original_instance_id", m_inst->id());

    auto* extracted = new InstanceImportTask(url, trusted, this, std::move(extraInfo));

    if (versionName.isEmpty()) {
        extracted->setName(m_inst->name());
    } else {
        extracted->setOriginalName(managedPack->name, versionName);
        extracted->setName(m_inst->name().replace(managedPack->versionName, versionName));
    }
    extracted->setGroup(APPLICATION->instances()->getInstanceGroup(m_inst->id()));
    extracted->setIcon(m_inst->config()->iconKey);
    extracted->setConfirmUpdate(false);

    // Run our task then handle the result
    auto didSucceed = runUpdateTask(extracted);
    onUpdateTaskCompleted(didSucceed);
}
