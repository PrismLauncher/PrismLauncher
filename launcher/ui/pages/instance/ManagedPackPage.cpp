// SPDX-FileCopyrightText: 2022 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ManagedPackPage.h"
#include "ui_ManagedPackPage.h"

#include <QListView>
#include <QProxyStyle>
#include <QStyleFactory>

#include "Application.h"
#include "BuildConfig.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "InstanceTask.h"
#include "Json.h"
#include "Markdown.h"

#include "modplatform/modrinth/ModrinthPackManifest.h"

#include "ui/InstanceWindow.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

/** This is just to override the combo box popup behavior so that the combo box doesn't take the whole screen.
 *  ... thanks Qt.
 */
class NoBigComboBoxStyle : public QProxyStyle {
    Q_OBJECT

   public:
    // clang-format off
    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
    {
        if (hint == QStyle::SH_ComboBox_Popup)
            return false;

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
    // clang-format on

    /**
     * Something about QProxyStyle and QStyle objects means they can't be free'd just
     * because all the widgets using them are gone.
     * They seems to be tied to the QApplicaiton lifecycle.
     * So make singletons tied to the lifetime of the application to clean them up and ensure they aren't
     * being remade over and over again, thus leaking memory.
     */
   public:
    static NoBigComboBoxStyle* getInstance(QStyle* style)
    {
        static QHash<QStyle*, NoBigComboBoxStyle*> s_singleton_instances_ = {};
        static std::mutex s_singleton_instances_mutex_;

        std::lock_guard<std::mutex> lock(s_singleton_instances_mutex_);
        auto inst_iter = s_singleton_instances_.constFind(style);
        NoBigComboBoxStyle* inst = nullptr;
        if (inst_iter == s_singleton_instances_.constEnd() || *inst_iter == nullptr) {
            inst = new NoBigComboBoxStyle(style);
            inst->setParent(APPLICATION);
            s_singleton_instances_.insert(style, inst);
            qDebug() << "QProxyStyle NoBigComboBox created for" << style->objectName() << style;
        } else {
            inst = *inst_iter;
        }
        return inst;
    }

   private:
    NoBigComboBoxStyle(QStyle* style) : QProxyStyle(style) {}

};

ManagedPackPage* ManagedPackPage::createPage(BaseInstance* inst, QString type, QWidget* parent)
{
    if (type == "modrinth")
        return new ModrinthManagedPackPage(inst, nullptr, parent);
    if (type == "flame" && (APPLICATION->capabilities() & Application::SupportsFlame))
        return new FlameManagedPackPage(inst, nullptr, parent);

    return new GenericManagedPackPage(inst, nullptr, parent);
}

ManagedPackPage::ManagedPackPage(BaseInstance* inst, InstanceWindow* instance_window, QWidget* parent)
    : QWidget(parent), m_instance_window(instance_window), ui(new Ui::ManagedPackPage), m_inst(inst)
{
    Q_ASSERT(inst);

    ui->setupUi(this);

    // NOTE: GTK2 themes crash with the proxy style.
    // This seems like an upstream bug, so there's not much else that can be done.
    if (!QStyleFactory::keys().contains("gtk2")){
        auto comboStyle = NoBigComboBoxStyle::getInstance(ui->versionsComboBox->style());
        ui->versionsComboBox->setStyle(comboStyle);
    }

    ui->reloadButton->setVisible(false);
    connect(ui->reloadButton, &QPushButton::clicked, this, [this](bool){
        ui->reloadButton->setVisible(false);

        m_loaded = false;
        // Pretend we're opening the page again
        openedImpl();
    });
}

ManagedPackPage::~ManagedPackPage()
{
    delete ui;
}

void ManagedPackPage::openedImpl()
{
    ui->packName->setText(m_inst->getManagedPackName());
    ui->packVersion->setText(m_inst->getManagedPackVersionName());
    ui->packOrigin->setText(tr("Website: <a href=%1>%2</a>    |    Pack ID: %3    |    Version ID: %4")
                                .arg(url(), displayName(), m_inst->getManagedPackID(), m_inst->getManagedPackVersionID()));

    parseManagedPack();
}

QString ManagedPackPage::displayName() const
{
    auto type = m_inst->getManagedPackType();
    if (type.isEmpty())
        return {};
    if (type == "flame")
        type = "CurseForge";
    return type.replace(0, 1, type[0].toUpper());
}

QIcon ManagedPackPage::icon() const
{
    return APPLICATION->getThemedIcon(m_inst->getManagedPackType());
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
    return m_inst->isManagedPack();
}

bool ManagedPackPage::runUpdateTask(InstanceTask* task)
{
    Q_ASSERT(task);

    unique_qobject_ptr<Task> wrapped_task(APPLICATION->instances()->wrapInstanceTask(task));

    connect(task, &Task::failed,
            [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(task, &Task::succeeded, [this, task]() {
        QStringList warnings = task->warnings();
        if (warnings.count())
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
    });
    connect(task, &Task::aborted, [this] {
        CustomMessageBox::selectable(this, tr("Task aborted"), tr("The task has been aborted by the user."), QMessageBox::Information)
            ->show();
    });

    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(task);

    return task->wasSuccessful();
}

void ManagedPackPage::suggestVersion()
{
    ui->updateButton->setText(tr("Update pack"));
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

ModrinthManagedPackPage::ModrinthManagedPackPage(BaseInstance* inst, InstanceWindow* instance_window, QWidget* parent)
    : ManagedPackPage(inst, instance_window, parent)
{
    Q_ASSERT(inst->isManagedPack());
    connect(ui->versionsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(suggestVersion()));
    connect(ui->updateButton, &QPushButton::pressed, this, &ModrinthManagedPackPage::update);
}

// MODRINTH

void ModrinthManagedPackPage::parseManagedPack()
{
    qDebug() << "Parsing Modrinth pack";

    // No need for the extra work because we already have everything we need.
    if (m_loaded)
        return;

    if (m_fetch_job && m_fetch_job->isRunning())
        m_fetch_job->abort();

    m_fetch_job.reset(new NetJob(QString("Modrinth::PackVersions(%1)").arg(m_inst->getManagedPackName()), APPLICATION->network()));
    auto response = std::make_shared<QByteArray>();

    QString id = m_inst->getManagedPackID();

    m_fetch_job->addNetAction(Net::Download::makeByteArray(QString("%1/project/%2/version").arg(BuildConfig.MODRINTH_PROD_URL, id), response));

    QObject::connect(m_fetch_job.get(), &NetJob::succeeded, this, [this, response, id] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            setFailState();

            return;
        }

        try {
            Modrinth::loadIndexedVersions(m_pack, doc);
        } catch (const JSONValidationError& e) {
            qDebug() << *response;
            qWarning() << "Error while reading modrinth modpack version: " << e.cause();

            setFailState();
            return;
        }

        // We block signals here so that suggestVersion() doesn't get called, causing an assertion fail.
        ui->versionsComboBox->blockSignals(true);
        ui->versionsComboBox->clear();
        ui->versionsComboBox->blockSignals(false);

        for (auto version : m_pack.versions) {
            QString name = version.version;

            if (!version.name.contains(version.version))
                name = QString("%1 — %2").arg(version.name, version.version);

            // NOTE: the id from version isn't the same id in the modpack format spec...
            // e.g. HexMC's 4.4.0 has versionId 4.0.0 in the modpack index..............
            if (version.version == m_inst->getManagedPackVersionName())
                name = tr("%1 (Current)").arg(name);


            ui->versionsComboBox->addItem(name, QVariant(version.id));
        }

        suggestVersion();

        m_loaded = true;
    });
    QObject::connect(m_fetch_job.get(), &NetJob::failed, this, &ModrinthManagedPackPage::setFailState);
    QObject::connect(m_fetch_job.get(), &NetJob::aborted, this, &ModrinthManagedPackPage::setFailState);

    ui->changelogTextBrowser->setText(tr("Fetching changelogs..."));

    m_fetch_job->start();
}

QString ModrinthManagedPackPage::url() const
{
    return "https://modrinth.com/mod/" + m_inst->getManagedPackID();
}

void ModrinthManagedPackPage::suggestVersion()
{
    auto index = ui->versionsComboBox->currentIndex();
    auto version = m_pack.versions.at(index);

    ui->changelogTextBrowser->setHtml(markdownToHTML(version.changelog.toUtf8()));

    ManagedPackPage::suggestVersion();
}

void ModrinthManagedPackPage::update()
{
    auto index = ui->versionsComboBox->currentIndex();
    auto version = m_pack.versions.at(index);

    QMap<QString, QString> extra_info;
    // NOTE: Don't use 'm_pack.id' here, since we didn't completely parse all the metadata for the pack, including this field.
    extra_info.insert("pack_id", m_inst->getManagedPackID());
    extra_info.insert("pack_version_id", version.id);
    extra_info.insert("original_instance_id", m_inst->id());

    auto extracted = new InstanceImportTask(version.download_url, this, std::move(extra_info));

    InstanceName inst_name(m_inst->getManagedPackName(), version.version);
    inst_name.setName(m_inst->name().replace(m_inst->getManagedPackVersionName(), version.version));
    extracted->setName(inst_name);

    extracted->setGroup(APPLICATION->instances()->getInstanceGroup(m_inst->id()));
    extracted->setIcon(m_inst->iconKey());
    extracted->setConfirmUpdate(false);

    auto did_succeed = runUpdateTask(extracted);

    if (m_instance_window && did_succeed)
        m_instance_window->close();
}

// FLAME

FlameManagedPackPage::FlameManagedPackPage(BaseInstance* inst, InstanceWindow* instance_window, QWidget* parent)
    : ManagedPackPage(inst, instance_window, parent)
{
    Q_ASSERT(inst->isManagedPack());
    connect(ui->versionsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(suggestVersion()));
    connect(ui->updateButton, &QPushButton::pressed, this, &FlameManagedPackPage::update);
}

void FlameManagedPackPage::parseManagedPack()
{
    qDebug() << "Parsing Flame pack";

    // We need to tell the user to redownload the pack, since we didn't save the required info previously
    if (m_inst->getManagedPackID().isEmpty()) {
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

        ui->changelogTextBrowser->setHtml(message);
        return;
    }

    // No need for the extra work because we already have everything we need.
    if (m_loaded)
        return;

    if (m_fetch_job && m_fetch_job->isRunning())
        m_fetch_job->abort();

    m_fetch_job.reset(new NetJob(QString("Flame::PackVersions(%1)").arg(m_inst->getManagedPackName()), APPLICATION->network()));
    auto response = std::make_shared<QByteArray>();

    QString id = m_inst->getManagedPackID();

    m_fetch_job->addNetAction(Net::Download::makeByteArray(QString("%1/mods/%2/files").arg(BuildConfig.FLAME_BASE_URL, id), response));

    QObject::connect(m_fetch_job.get(), &NetJob::succeeded, this, [this, response, id] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Flame at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            setFailState();

            return;
        }

        try {
            auto obj = doc.object();
            auto data = Json::ensureArray(obj, "data");
            Flame::loadIndexedPackVersions(m_pack, data);
        } catch (const JSONValidationError& e) {
            qDebug() << *response;
            qWarning() << "Error while reading flame modpack version: " << e.cause();

            setFailState();
            return;
        }

        // We block signals here so that suggestVersion() doesn't get called, causing an assertion fail.
        ui->versionsComboBox->blockSignals(true);
        ui->versionsComboBox->clear();
        ui->versionsComboBox->blockSignals(false);

        for (auto version : m_pack.versions) {
            QString name = version.version;

            if (version.fileId == m_inst->getManagedPackVersionID().toInt())
                name = tr("%1 (Current)").arg(name);

            ui->versionsComboBox->addItem(name, QVariant(version.fileId));
        }

        suggestVersion();

        m_loaded = true;
    });
    QObject::connect(m_fetch_job.get(), &NetJob::failed, this, &FlameManagedPackPage::setFailState);
    QObject::connect(m_fetch_job.get(), &NetJob::aborted, this, &FlameManagedPackPage::setFailState);

    m_fetch_job->start();
}

QString FlameManagedPackPage::url() const
{
    // FIXME: We should display the websiteUrl field, but this requires doing the API request first :(
    return {};
}

void FlameManagedPackPage::suggestVersion()
{
    auto index = ui->versionsComboBox->currentIndex();
    auto version = m_pack.versions.at(index);

    ui->changelogTextBrowser->setHtml(m_api.getModFileChangelog(m_inst->getManagedPackID().toInt(), version.fileId));

    ManagedPackPage::suggestVersion();
}

void FlameManagedPackPage::update()
{
    auto index = ui->versionsComboBox->currentIndex();
    auto version = m_pack.versions.at(index);

    QMap<QString, QString> extra_info;
    extra_info.insert("pack_id", m_inst->getManagedPackID());
    extra_info.insert("pack_version_id", QString::number(version.fileId));
    extra_info.insert("original_instance_id", m_inst->id());

    auto extracted = new InstanceImportTask(version.downloadUrl, this, std::move(extra_info));

    extracted->setName(m_inst->name());
    extracted->setGroup(APPLICATION->instances()->getInstanceGroup(m_inst->id()));
    extracted->setIcon(m_inst->iconKey());
    extracted->setConfirmUpdate(false);

    auto did_succeed = runUpdateTask(extracted);

    if (m_instance_window && did_succeed)
        m_instance_window->close();
}

#include "ManagedPackPage.moc"
