#include "ModEditDialogCommon.h"
#include "CustomMessageBox.h"
#include <QDesktopServices>
#include <QMessageBox>
#include <QString>
#include <QUrl>
bool lastfirst(QModelIndexList &list, int &first, int &last)
{
	if (!list.size())
		return false;
	first = last = list[0].row();
	for (auto item : list)
	{
		int row = item.row();
		if (row < first)
			first = row;
		if (row > last)
			last = row;
	}
	return true;
}

void showWebsiteForMod(QWidget *parentDlg, Mod &m)
{
	QString url = m.homeurl();
	if (url.size())
	{
		// catch the cases where the protocol is missing
		if(!url.startsWith("http"))
		{
			url = "http://" + url;
		}
		QDesktopServices::openUrl(url);
	}
	else
	{
		CustomMessageBox::selectable(parentDlg, parentDlg->tr("How sad!"),
									 parentDlg->tr("The mod author didn't provide a website link for this mod."),
									 QMessageBox::Warning);
	}
}
