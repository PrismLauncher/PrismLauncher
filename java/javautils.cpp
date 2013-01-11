#include "multimc_pragma.h"
#include "classfile.h"
#include "javautils.h"
//#include <wx/zipstrm.h>
#include <memory>
//#include <wx/wfstream.h>
//#include "mcversionlist.h"

namespace javautils
{
QString GetMinecraftJarVersion(QString jar)
{
	return "Unknown";
	/*
	wxString fullpath = jar.GetFullPath();
	wxString version = MCVer_Unknown;
	if(!jar.FileExists())
		return version;
	std::auto_ptr<wxZipEntry> entry;
	// convert the local name we are looking for into the internal format
	wxString name = wxZipEntry::GetInternalName("net/minecraft/client/Minecraft.class",wxPATH_UNIX);

	// open the zip
	wxFFileInputStream inStream(jar.GetFullPath());
	wxZipInputStream zipIn(inStream);

	// call GetNextEntry() until the required internal name is found
	do
	{
		entry.reset(zipIn.GetNextEntry());
	}
	while (entry.get() != NULL && entry->GetInternalName() != name);
	auto myentry = entry.get();
	if (myentry == NULL)
		return version;
	
	// we got the entry, read the data
	std::size_t size = myentry->GetSize();
	char *classdata = new char[size];
	zipIn.Read(classdata,size);
	try
	{
		char * temp = classdata;
		java::classfile Minecraft_jar(temp,size);
		auto cnst = Minecraft_jar.constants;
		auto iter = cnst.begin();
		while (iter != cnst.end())
		{
			const java::constant & constant = *iter;
			if(constant.type != java::constant::j_string_data)
			{
				iter++;
				continue;
			}
			auto & str = constant.str_data;
			const char * lookfor = "Minecraft Minecraft "; // length = 20
			if(str.compare(0,20,lookfor) == 0)
			{
				version = str.substr(20).data();
				break;
			}
			iter++;
		}
	} catch(java::classfile_exception &){}
	delete[] classdata;
	return version;
	*/
}
}