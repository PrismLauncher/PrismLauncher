#include "UpdateScript.h"

#include "Log.h"
#include "StringUtils.h"

#include "tinyxml/tinyxml.h"

std::string elementText(const TiXmlElement* element)
{
	if (!element)
	{
		return std::string();
	}
	return element->GetText();
}

UpdateScript::UpdateScript()
{
}

void UpdateScript::parse(const std::string& path)
{
	m_path.clear();

	TiXmlDocument document(path);
	if (document.LoadFile())
	{
		m_path = path;

		LOG(Info,"Loaded script from " + path);

		const TiXmlElement* updateNode = document.RootElement();
		parseUpdate(updateNode);
	}
	else
	{
		LOG(Error,"Unable to load script " + path);
	}
}

bool UpdateScript::isValid() const
{
	return !m_path.empty();
}

void UpdateScript::parseUpdate(const TiXmlElement* updateNode)
{
	const TiXmlElement* installNode = updateNode->FirstChildElement("install");
	if (installNode)
	{
		const TiXmlElement* installFileNode = installNode->FirstChildElement("file");
		while (installFileNode)
		{
			m_filesToInstall.push_back(parseFile(installFileNode));
			installFileNode = installFileNode->NextSiblingElement("file");
		}
	}

	const TiXmlElement* uninstallNode = updateNode->FirstChildElement("uninstall");
	if (uninstallNode)
	{
		const TiXmlElement* uninstallFileNode = uninstallNode->FirstChildElement("file");
		while (uninstallFileNode)
		{
			m_filesToUninstall.push_back(uninstallFileNode->GetText());
			uninstallFileNode = uninstallFileNode->NextSiblingElement("file");
		}
	}
}

UpdateScriptFile UpdateScript::parseFile(const TiXmlElement* element)
{
	UpdateScriptFile file;
	file.path = elementText(element->FirstChildElement("name"));

	std::string modeString = elementText(element->FirstChildElement("permissions"));
	sscanf(modeString.c_str(),"%i",&file.permissions);

	file.linkTarget = elementText(element->FirstChildElement("target"));
	file.isMainBinary = strToBool(elementText(element->FirstChildElement("is-main-binary")));
	return file;
}

const std::vector<UpdateScriptFile>& UpdateScript::filesToInstall() const
{
	return m_filesToInstall;
}

const std::vector<std::string>& UpdateScript::filesToUninstall() const
{
	return m_filesToUninstall;
}

const std::string UpdateScript::path() const
{
	return m_path;
}

