#pragma once

#include <string>
#include <vector>

class TiXmlElement;

/** Represents a package containing one or more
  * files for an update.
  */
class UpdateScriptPackage
{
	public:
		UpdateScriptPackage()
		: size(0)
		{}

		std::string name;
		std::string sha1;
		std::string source;
		int size;

		bool operator==(const UpdateScriptPackage& other) const
		{
			return name == other.name &&
			       sha1 == other.sha1 &&
			       source == other.source &&
			       size == other.size;
		}
};

/** Represents a file to be installed as part of an update. */
class UpdateScriptFile
{
	public:
		UpdateScriptFile()
		: permissions(0)
		{}

		/// Path to copy from.
		std::string source;
		/// The path to copy to.
		std::string dest;

		/** The permissions for this file, specified
		  * using the standard Unix mode_t values.
		  */
		int permissions;

		bool operator==(const UpdateScriptFile& other) const
		{
			return source == other.source &&
			       dest == other.dest &&
			       permissions == other.permissions;
		}
};

/** Stores information about the files included in an update, parsed from an XML file. */
class UpdateScript
{
	public:
		UpdateScript();

		/** Initialize this UpdateScript with the script stored
		  * in the XML file at @p path.
		  */
		void parse(const std::string& path);

		bool isValid() const;
		const std::string path() const;
		const std::vector<UpdateScriptFile>& filesToInstall() const;
		const std::vector<std::string>& filesToUninstall() const;

	private:
		void parseUpdate(const TiXmlElement* element);
		UpdateScriptFile parseFile(const TiXmlElement* element);

		std::string m_path;
		std::vector<UpdateScriptFile> m_filesToInstall;
		std::vector<std::string> m_filesToUninstall;
};

