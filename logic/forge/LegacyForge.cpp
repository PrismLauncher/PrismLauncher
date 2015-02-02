/* Copyright 2013-2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LegacyForge.h"

MinecraftForge::MinecraftForge(const QString &file) : Mod(file)
{
}

bool MinecraftForge::FixVersionIfNeeded(QString newVersion)
{/*
	wxString reportedVersion = GetModVersion();
	if(reportedVersion == "..." || reportedVersion.empty())
	{
		std::auto_ptr<wxFFileInputStream> in(new wxFFileInputStream("forge.zip"));
		wxTempFileOutputStream out("forge.zip");
		wxTextOutputStream textout(out);
		wxZipInputStream inzip(*in);
		wxZipOutputStream outzip(out);
		std::auto_ptr<wxZipEntry> entry;
		// preserve metadata
		outzip.CopyArchiveMetaData(inzip);
		// copy all entries
		while (entry.reset(inzip.GetNextEntry()), entry.get() != NULL)
			if (!outzip.CopyEntry(entry.release(), inzip))
				return false;
		// release last entry
		in.reset();
		outzip.PutNextEntry("forgeversion.properties");

		wxStringTokenizer tokenizer(newVersion,".");
		wxString verFile;
		verFile << wxString("forge.major.number=") << tokenizer.GetNextToken() << "\n";
		verFile << wxString("forge.minor.number=") << tokenizer.GetNextToken() << "\n";
		verFile << wxString("forge.revision.number=") << tokenizer.GetNextToken() << "\n";
		verFile << wxString("forge.build.number=") << tokenizer.GetNextToken() << "\n";
		auto buf = verFile.ToUTF8();
		outzip.Write(buf.data(), buf.length());
		// check if we succeeded
		return inzip.Eof() && outzip.Close() && out.Commit();
	}
	*/
	return true;
}
