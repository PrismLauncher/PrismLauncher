//
// Created by robotbrain on 3/26/16.
//

#include "WonkoClient.h"
#include <QApplication>
#include <QDebug>
#include <QtWidgets/QInputDialog>
#include <QtGui/QDesktopServices>
#include <QDir>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	if (a.arguments().contains("-d"))
	{
		int i = a.arguments().lastIndexOf("-d") + 1;
		if (i < a.arguments().length())
		{
			QDir dir = QDir::current();
			dir.cd(a.arguments()[i]);
			QDir::setCurrent(dir.absolutePath());
			qDebug() << "Using " << dir.absolutePath();
		}
	}
	MMCC.initGlobalSettings();
	MMCC.registerLists();
	return a.exec();
}
