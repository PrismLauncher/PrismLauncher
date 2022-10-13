#include "ManagedPackPage.h"
#include "ui_ManagedPackPage.h"

#include <QListView>
#include <QProxyStyle>

#include "Application.h"

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
}

QString ModrinthManagedPackPage::url() const
{
    return {};
}

void ModrinthManagedPackPage::suggestVersion()
{
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
