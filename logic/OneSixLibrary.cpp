#include "OneSixLibrary.h"
#include "OneSixRule.h"
#include "OpSys.h"
void OneSixLibrary::finalize()
{
	QStringList parts = m_name.split ( ':' );
	QString relative = parts[0];
	relative.replace ( '.','/' );
	relative += '/' + parts[1] + '/' + parts[2] + '/' + parts[1] + '-' + parts[2];
	
	if ( !m_is_native )
		relative += ".jar";
	else
	{
		if ( m_native_suffixes.contains ( currentSystem ) )
		{
			relative += "-" + m_native_suffixes[currentSystem] + ".jar";
		}
		else
		{
			// really, bad.
			relative += ".jar";
		}
	}
	
	m_decentname = parts[1];
	m_decentversion = parts[2];
	m_storage_path = relative;
	m_download_path = m_base_url + relative;
	
	if ( m_rules.empty() )
	{
		m_is_active = true;
	}
	else
	{
		RuleAction result = Disallow;
		for ( auto rule: m_rules )
		{
			RuleAction temp = rule->apply ( this );
			if ( temp != Defer )
				result = temp;
		}
		m_is_active = ( result == Allow );
	}
	if ( m_is_native )
	{
		m_is_active = m_is_active && m_native_suffixes.contains ( currentSystem );
		m_decenttype = "Native";
	}
	else
	{
		m_decenttype = "Java";
	}
}

void OneSixLibrary::setName ( QString name )
{
	m_name = name;
}
void OneSixLibrary::setBaseUrl ( QString base_url )
{
	m_base_url = base_url;
}
void OneSixLibrary::setIsNative()
{
	m_is_native = true;
}
void OneSixLibrary::addNative ( OpSys os, QString suffix )
{
	m_is_native = true;
	m_native_suffixes[os] = suffix;
}
void OneSixLibrary::setRules ( QList< QSharedPointer< Rule > > rules )
{
	m_rules = rules;
}
bool OneSixLibrary::isActive()
{
	return m_is_active;
}
bool OneSixLibrary::isNative()
{
	return m_is_native;
}
QString OneSixLibrary::downloadPath()
{
	return m_download_path;
}
QString OneSixLibrary::storagePath()
{
	return m_storage_path;
}
