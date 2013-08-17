#include "include/userutils.h"

#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

#include "include/osutils.h"
#include "include/pathutils.h"

// Win32 crap
#if WINDOWS

#include <windows.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <shlobj.h>

bool called_coinit = false;

HRESULT CreateLink(LPCSTR linkPath, LPCSTR targetPath, LPCSTR args)
{
	HRESULT hres;
	
	if (!called_coinit)
	{
		hres = CoInitialize(NULL);
		called_coinit = true;
		
		if (!SUCCEEDED(hres))
		{
			qWarning("Failed to initialize COM. Error 0x%08X", hres);
			return hres;
		}
	}
	
	
	IShellLink* link;
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&link);
	
	if (SUCCEEDED(hres))
	{
		IPersistFile* persistFile;
		
		link->SetPath(targetPath);
		link->SetArguments(args);
		
		hres = link->QueryInterface(IID_IPersistFile, (LPVOID*)&persistFile);
		if (SUCCEEDED(hres))
		{
			WCHAR wstr[MAX_PATH];
			
			MultiByteToWideChar(CP_ACP, 0, linkPath, -1, wstr, MAX_PATH);
			
			hres = persistFile->Save(wstr, TRUE);
			persistFile->Release();
		}
		link->Release();
	}
	return hres;
}

#endif

QString Util::getDesktopDir()
{
	return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

// Cross-platform Shortcut creation
bool Util::createShortCut(QString location, QString dest, QStringList args, QString name, QString icon)
{
#if LINUX
	location = PathCombine(location, name + ".desktop");
	qDebug("location: %s", qPrintable(location));
	
	QFile f(location);
	f.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream stream(&f);
	
	QString argstring;
	if (!args.empty())
		argstring = " '" + args.join("' '") + "'";
	
	stream << "[Desktop Entry]" << "\n";
	stream << "Type=Application" << "\n";
	stream << "TryExec=" << dest.toLocal8Bit() << "\n";
	stream << "Exec=" << dest.toLocal8Bit() << argstring.toLocal8Bit() << "\n";
	stream << "Name=" << name.toLocal8Bit() << "\n";
	stream << "Icon=" << icon.toLocal8Bit() << "\n";
	
	stream.flush();
	f.close();
	
	f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
	
	return true;
#elif WINDOWS
	// TODO: Fix
//	QFile file(PathCombine(location, name + ".lnk"));
//	WCHAR *file_w;
//	WCHAR *dest_w;
//	WCHAR *args_w;
//	file.fileName().toWCharArray(file_w);
//	dest.toWCharArray(dest_w);
	
//	QString argStr;
//	for (int i = 0; i < args.count(); i++)
//	{
//		argStr.append(args[i]);
//		argStr.append(" ");
//	}
//	argStr.toWCharArray(args_w);
	
//	return SUCCEEDED(CreateLink(file_w, dest_w, args_w));
	return false;
#else
	qWarning("Desktop Shortcuts not supported on your platform!");
	return false;
#endif
}
