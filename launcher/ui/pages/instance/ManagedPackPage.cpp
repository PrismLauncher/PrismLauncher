#include "ManagedPackPage.h"
#include "ui_ManagedPackPage.h"

#include <QListView>
#include <QProxyStyle>

#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"

#include "modplatform/modrinth/ModrinthPackManifest.h"

/** This is just to override the combo box popup behavior so that the combo box doesn't take the whole screen.
 *  ... thanks Qt.
 */
class NoBigComboBoxStyle : public QProxyStyle {
    Q_OBJECT

   public:
    NoBigComboBoxStyle(QStyle* style) : QProxyStyle(style) {}

    // clang-format off
    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
    {
        if (hint == QStyle::SH_ComboBox_Popup)
            return false;

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
    // clang-format on
};

ManagedPackPage* ManagedPackPage::createPage(BaseInstance* inst, QString type, QWidget* parent)
{
    if (type == "modrinth")
        return new ModrinthManagedPackPage(inst, parent);
    if (type == "flame")
        return new FlameManagedPackPage(inst, parent);

    return new GenericManagedPackPage(inst, parent);
}

ManagedPackPage::ManagedPackPage(BaseInstance* inst, QWidget* parent) : QWidget(parent), ui(new Ui::ManagedPackPage), m_inst(inst)
{
    Q_ASSERT(inst);

    ui->setupUi(this);

    ui->versionsComboBox->setStyle(new NoBigComboBoxStyle(ui->versionsComboBox->style()));
}

ManagedPackPage::~ManagedPackPage()
{
    delete ui;
}

void ManagedPackPage::openedImpl()
{
    ui->packName->setText(m_inst->getManagedPackName());
    ui->packVersion->setText(m_inst->getManagedPackVersionName());
    ui->packOrigin->setText(tr("Website: %1    |    Pack ID: %2    |    Version ID: %3")
                                .arg(displayName(), m_inst->getManagedPackID(), m_inst->getManagedPackVersionID()));

    parseManagedPack();
}

QString ManagedPackPage::displayName() const
{
    auto type = m_inst->getManagedPackType();
    if (type.isEmpty())
        return {};
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

ModrinthManagedPackPage::ModrinthManagedPackPage(BaseInstance* inst, QWidget* parent) : ManagedPackPage(inst, parent)
{
    Q_ASSERT(inst->isManagedPack());
    connect(ui->versionsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(suggestVersion()));
}

void ModrinthManagedPackPage::parseManagedPack()
{
    qDebug() << "Parsing Modrinth pack";

    auto netJob = new NetJob(QString("Modrinth::PackVersions(%1)").arg(m_inst->getManagedPackName()), APPLICATION->network());
    auto response = new QByteArray();

    QString id = m_inst->getManagedPackID();

    netJob->addNetAction(Net::Download::makeByteArray(QString("%1/project/%2/version").arg(BuildConfig.MODRINTH_PROD_URL, id), response));

    QObject::connect(netJob, &NetJob::succeeded, this, [this, response, id] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        try {
            Modrinth::loadIndexedVersions(m_pack, doc);
        } catch (const JSONValidationError& e) {
            qDebug() << *response;
            qWarning() << "Error while reading modrinth modpack version: " << e.cause();
        }

        for (auto version : m_pack.versions) {
            QString name;

            if (!version.name.contains(version.version))
                name = QString("%1 — %2").arg(version.name, version.version);
            else
                name = version.name;

            // NOTE: the id from version isn't the same id in the modpack format spec...
            // e.g. HexMC's 4.4.0 has versionId 4.0.0 in the modpack index..............
            if (version.version == m_inst->getManagedPackVersionName())
                name.append(tr(" (Current)"));

            ui->versionsComboBox->addItem(name, QVariant(version.id));
        }

        suggestVersion();

        m_loaded = true;
    });
    QObject::connect(netJob, &NetJob::finished, this, [response, netJob] {
        netJob->deleteLater();
        delete response;
    });
    netJob->start();
}

QString ModrinthManagedPackPage::url() const
{
    return {};
}

void ModrinthManagedPackPage::suggestVersion()
{
    auto index = ui->versionsComboBox->currentIndex();
    auto version = m_pack.versions.at(index);

    ui->changelogTextBrowser->setText(version.changelog);
}

FlameManagedPackPage::FlameManagedPackPage(BaseInstance* inst, QWidget* parent) : ManagedPackPage(inst, parent)
{
    Q_ASSERT(inst->isManagedPack());
    connect(ui->versionsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(suggestVersion()));
}

void FlameManagedPackPage::parseManagedPack()
{
}

QString FlameManagedPackPage::url() const
{
    return {};
}

void FlameManagedPackPage::suggestVersion()
{
}

#include "ManagedPackPage.moc"
