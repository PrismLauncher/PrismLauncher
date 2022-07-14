#include "InstanceTask.h"

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
