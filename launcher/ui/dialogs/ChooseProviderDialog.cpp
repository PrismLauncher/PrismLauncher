#include "ChooseProviderDialog.h"
#include "ui_ChooseProviderDialog.h"

#include <QPushButton>
#include <QRadioButton>

#include "modplatform/ModIndex.h"

static ModPlatform::ProviderCapabilities ProviderCaps;

ChooseProviderDialog::ChooseProviderDialog(QWidget* parent, bool single_choice, bool allow_skipping)
    : QDialog(parent), ui(new Ui::ChooseProviderDialog)
{
    ui->setupUi(this);

    addProviders();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providers.button(0)->click();

    connect(ui->skipOneButton, &QPushButton::clicked, this, &ChooseProviderDialog::skipOne);
    connect(ui->skipAllButton, &QPushButton::clicked, this, &ChooseProviderDialog::skipAll);

    connect(ui->confirmOneButton, &QPushButton::clicked, this, &ChooseProviderDialog::confirmOne);
    connect(ui->confirmAllButton, &QPushButton::clicked, this, &ChooseProviderDialog::confirmAll);

    if (single_choice) {
        ui->providersLayout->removeWidget(ui->skipAllButton);
        ui->providersLayout->removeWidget(ui->confirmAllButton);
    }

    if (!allow_skipping) {
        ui->providersLayout->removeWidget(ui->skipOneButton);
        ui->providersLayout->removeWidget(ui->skipAllButton);
    }
}

ChooseProviderDialog::~ChooseProviderDialog()
{
    delete ui;
}

void ChooseProviderDialog::setDescription(QString desc)
{
    ui->explanationLabel->setText(desc);
}

void ChooseProviderDialog::skipOne()
{
    reject();
}
void ChooseProviderDialog::skipAll()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response.skip_all = true;
    reject();
}

void ChooseProviderDialog::confirmOne()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response.chosen = getSelectedProvider();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response.try_others = ui->tryOthersCheckbox->isChecked();
    accept();
}
void ChooseProviderDialog::confirmAll()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response.chosen = getSelectedProvider();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response.confirhello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_all = true;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response.try_others = ui->tryOthersCheckbox->isChecked();
    accept();
}

auto ChooseProviderDialog::getSelectedProvider() const -> ModPlatform::Provider
{
    return ModPlatform::Provider(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providers.checkedId());
}

void ChooseProviderDialog::addProviders()
{
    int btn_index = 0;
    QRadioButton* btn;

    for (auto& provider : { ModPlatform::Provider::MODRINTH, ModPlatform::Provider::FLAME }) {
        btn = new QRadioButton(ProviderCaps.readableName(provider), this);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providers.addButton(btn, btn_index++);
        ui->providersLayout->addWidget(btn);
    }
}

void ChooseProviderDialog::disableInput()
{
    for (auto& btn : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providers.buttons())
        btn->setEnabled(false);

    ui->skipOneButton->setEnabled(false);
    ui->skipAllButton->setEnabled(false);
    ui->confirmOneButton->setEnabled(false);
    ui->confirmAllButton->setEnabled(false);
}
