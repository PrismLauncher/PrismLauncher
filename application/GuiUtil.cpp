#include "GuiUtil.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QApplication>
#include <QFileDialog>

#include "dialogs/ProgressDialog.h"
#include "net/PasteUpload.h"
#include "dialogs/CustomMessageBox.h"

#include "MultiMC.h"
#include <settings/SettingsObject.h>

void GuiUtil::uploadPaste(const QString &text, QWidget *parentWidget)
{
	ProgressDialog dialog(parentWidget);
	std::unique_ptr<PasteUpload> paste(new PasteUpload(parentWidget, text));

	if (!paste->validateText())
	{
		CustomMessageBox::selectable(
			parentWidget, QObject::tr("Upload failed"),
			QObject::tr("The log file is too big. You'll have to upload it manually."),
			QMessageBox::Warning)->exec();
		return;
	}

	dialog.exec(paste.get());
	if (!paste->successful())
	{
		CustomMessageBox::selectable(parentWidget, QObject::tr("Upload failed"),
									 paste->failReason(), QMessageBox::Critical)->exec();
	}
	else
	{
		const QString link = paste->pasteLink();
		setClipboardText(link);
		QDesktopServices::openUrl(link);
		CustomMessageBox::selectable(
			parentWidget, QObject::tr("Upload finished"),
			QObject::tr("The <a href=\"%1\">link to the uploaded log</a> has been opened in "
						"the default "
						"browser and placed in your clipboard.").arg(link),
			QMessageBox::Information)->exec();
	}
}

void GuiUtil::setClipboardText(const QString &text)
{
	QApplication::clipboard()->setText(text);
}

QStringList GuiUtil::BrowseForMods(QString context, QString caption, QString filter,
								   QWidget *parentWidget)
{
	static QMap<QString, QString> savedPaths;

	QFileDialog w(parentWidget, caption);
	QSet<QString> locations;
	QString modsFolder = MMC->settings()->get("CentralModsDir").toString();
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
	urls.append(QUrl::fromLocalFile(modsFolder));

	w.setFileMode(QFileDialog::ExistingFiles);
	w.setAcceptMode(QFileDialog::AcceptOpen);
	w.setNameFilter(filter);
	if(savedPaths.contains(context))
	{
		w.setDirectory(savedPaths[context]);
	}
	else
	{
		w.setDirectory(modsFolder);
	}
	w.setSidebarUrls(urls);

	if (w.exec())
	{
		savedPaths[context] = w.directory().absolutePath();
		return w.getOpenFileNames();
	}
	savedPaths[context] = w.directory().absolutePath();
	return {};
}
