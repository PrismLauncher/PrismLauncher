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
	PasteUpload *paste = new PasteUpload(parentWidget, text);
	dialog.exec(paste);
	if (!paste->successful())
	{
		CustomMessageBox::selectable(parentWidget, "Upload failed", paste->failReason(),
									 QMessageBox::Critical)->exec();
	}
	else
	{
		const QString link = paste->pasteLink();
		setClipboardText(link);
		QDesktopServices::openUrl(link);
		CustomMessageBox::selectable(
			parentWidget, QObject::tr("Upload finished"),
			QObject::tr("The <a href=\"%1\">link to the uploaded log</a> has been opened in the default "
			   "browser and placed in your clipboard.").arg(link),
			QMessageBox::Information)->exec();
	}
	delete paste;
}

void GuiUtil::setClipboardText(const QString &text)
{
	QApplication::clipboard()->setText(text);
}
