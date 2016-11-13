#include "GuiUtil.h"

#include <QClipboard>
#include <QApplication>
#include <QFileDialog>

#include "dialogs/ProgressDialog.h"
#include "net/PasteUpload.h"
#include "dialogs/CustomMessageBox.h"

#include "MultiMC.h"
#include <settings/SettingsObject.h>
#include <DesktopServices.h>
#include <BuildConfig.h>

QString GuiUtil::uploadPaste(const QString &text, QWidget *parentWidget)
{
	ProgressDialog dialog(parentWidget);
	auto APIKeySetting = MMC->settings()->get("PasteEEAPIKey").toString();
	if(APIKeySetting == "multimc")
	{
		APIKeySetting = BuildConfig.PASTE_EE_KEY;
	}
	std::unique_ptr<PasteUpload> paste(new PasteUpload(parentWidget, text, APIKeySetting));

	if (!paste->validateText())
	{
		CustomMessageBox::selectable(
			parentWidget, QObject::tr("Upload failed"),
			QObject::tr("The log file is too big. You'll have to upload it manually."),
			QMessageBox::Warning)->exec();
		return QString();
	}

	dialog.execWithTask(paste.get());
	if (!paste->successful())
	{
		CustomMessageBox::selectable(parentWidget, QObject::tr("Upload failed"),
									 paste->failReason(), QMessageBox::Critical)->exec();
		return QString();
	}
	else
	{
		const QString link = paste->pasteLink();
		setClipboardText(link);
		CustomMessageBox::selectable(
			parentWidget, QObject::tr("Upload finished"),
			QObject::tr("The <a href=\"%1\">link to the uploaded log</a> has been opened in "
						"the default "
						"browser and placed in your clipboard.").arg(link),
			QMessageBox::Information)->exec();
		return link;
	}
}

void GuiUtil::setClipboardText(const QString &text)
{
	QApplication::clipboard()->setText(text);
}


QStringList GuiUtil::BrowseForFiles(QString context, QString caption, QString filter, QString defaultPath, QWidget *parentWidget)
{
	static QMap<QString, QString> savedPaths;

	QFileDialog w(parentWidget, caption);
	QSet<QString> locations;
	auto f = [&](QStandardPaths::StandardLocation l)
	{
		QString location = QStandardPaths::writableLocation(l);
		QFileInfo finfo(location);
		if (!finfo.exists())
			return;
		locations.insert(location);
	};
	f(QStandardPaths::DesktopLocation);
	f(QStandardPaths::DocumentsLocation);
	f(QStandardPaths::DownloadLocation);
	f(QStandardPaths::HomeLocation);
	QList<QUrl> urls;
	for (auto location : locations)
	{
		urls.append(QUrl::fromLocalFile(location));
	}
	urls.append(QUrl::fromLocalFile(defaultPath));

	w.setFileMode(QFileDialog::ExistingFiles);
	w.setAcceptMode(QFileDialog::AcceptOpen);
	w.setNameFilter(filter);

	QString pathToOpen;
	if(savedPaths.contains(context))
	{
		pathToOpen = savedPaths[context];
	}
	else
	{
		pathToOpen = defaultPath;
	}
	if(!pathToOpen.isEmpty())
	{
		QFileInfo finfo(pathToOpen);
		if(finfo.exists() && finfo.isDir())
		{
			w.setDirectory(finfo.absoluteFilePath());
		}
	}

	w.setSidebarUrls(urls);

	if (w.exec())
	{
		savedPaths[context] = w.directory().absolutePath();
		return w.selectedFiles();
	}
	savedPaths[context] = w.directory().absolutePath();
	return {};
}
