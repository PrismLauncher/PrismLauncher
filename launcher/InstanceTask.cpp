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

QString InstanceName::name() const
{
    if (!m_modified_name.isEmpty())
        return modifiedName();
    return QString("%1 %2").arg(m_original_name, m_original_version);
}

QString InstanceName::originalName() const
{
    return m_original_name;
}

QString InstanceName::modifiedName() const
{
    if (!m_modified_name.isEmpty())
        return m_modified_name;
    return m_original_name;
}

QString InstanceName::version() const
{
    return m_original_version;
}

void InstanceName::setName(InstanceName& other)
{
    m_original_name = other.m_original_name;
    m_original_version = other.m_original_version;
    m_modified_name = other.m_modified_name;
}

InstanceTask::InstanceTask() : Task(), InstanceName() {}
