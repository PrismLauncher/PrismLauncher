#include "InstanceTask.h"

#include "ui/dialogs/CustomMessageBox.h"

InstanceNameChange askForChangingInstanceName(QWidget* parent, const QString& old_name, const QString& new_name)
{
    auto dialog =
        CustomMessageBox::selectable(parent, QObject::tr("Change instance name"),
                                     QObject::tr("The instance's name seems to include the old version. Would you like to update it?\n\n"
                                                 "Old name: %1\n"
                                                 "New name: %2")
                                         .arg(old_name, new_name),
                                     QMessageBox::Question, QMessageBox::No | QMessageBox::Yes);
    auto result = dialog->exec();

    if (result == QMessageBox::Yes)
        return InstanceNameChange::ShouldChange;
    return InstanceNameChange::ShouldKeep;
}

ShouldUpdate askIfShouldUpdate(QWidget *parent, QString original_version_name)
{
    auto info = CustomMessageBox::selectable(
        parent, QObject::tr("Similar modpack was found!"),
        QObject::tr("One or more of your instances are from this same modpack%1. Do you want to create a "
           "separate instance, or update the existing one?\n\nNOTE: Make sure you made a backup of your important instance data before "
           "updating, as worlds can be corrupted and some configuration may be lost (due to pack overrides).")
            .arg(original_version_name),
        QMessageBox::Information, QMessageBox::Ok | QMessageBox::Reset | QMessageBox::Abort);
    info->setButtonText(QMessageBox::Ok, QObject::tr("Update existing instance"));
    info->setButtonText(QMessageBox::Abort, QObject::tr("Create new instance"));
    info->setButtonText(QMessageBox::Reset, QObject::tr("Cancel"));

    info->exec();

    if (info->clickedButton() == info->button(QMessageBox::Ok))
        return ShouldUpdate::Update;
    if (info->clickedButton() == info->button(QMessageBox::Abort))
        return ShouldUpdate::SkipUpdating;
    return ShouldUpdate::Cancel;

}

QString InstanceName::name() const
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name.isEmpty())
        return modifiedName();
    return QString("%1 %2").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_version);
}

QString InstanceName::originalName() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name;
}

QString InstanceName::modifiedName() const
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name.isEmpty())
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name;
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name;
}

QString InstanceName::version() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_version;
}

void InstanceName::setName(InstanceName& other)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name = other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_name;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_version = other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name = other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modified_name;
}

InstanceTask::InstanceTask() : Task(), InstanceName() {}
