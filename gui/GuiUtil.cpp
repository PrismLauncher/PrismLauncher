#include "GuiUtil.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QApplication>

#include "dialogs/ProgressDialog.h"
#include "logic/net/PasteUpload.h"
#include "dialogs/CustomMessageBox.h"

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
