#include "ModEditDialogCommon.h"
#include "CustomMessageBox.h"
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
		if (!url.startsWith("http"))
		{
			url = "http://" + url;
		}
		QDesktopServices::openUrl(url);
	}
	else
	{
		CustomMessageBox::selectable(
			parentDlg, QObject::tr("How sad!"),
			QObject::tr("The mod author didn't provide a website link for this mod."),
			QMessageBox::Warning);
	}
}