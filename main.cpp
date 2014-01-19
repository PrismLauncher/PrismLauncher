#include "MultiMC.h"
#include "gui/MainWindow.h"

int main_gui(MultiMC &app)
{
	// show main window
	QIcon::setThemeName("multimc");
	MainWindow mainWin;
	mainWin.restoreState(QByteArray::fromBase64(MMC->settings()->get("MainWindowState").toByteArray()));
	mainWin.restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("MainWindowGeometry").toByteArray()));
	mainWin.show();
	mainWin.checkMigrateLegacyAssets();
	mainWin.checkSetDefaultJava();
	return app.exec();
}

int main(int argc, char *argv[])
{
	// initialize Qt
	MultiMC app(argc, argv);

	Q_INIT_RESOURCE(instances);
	Q_INIT_RESOURCE(multimc);
	Q_INIT_RESOURCE(backgrounds);

	switch (app.status())
	{
	case MultiMC::Initialized:
		return main_gui(app);
	case MultiMC::Failed:
		return 1;
	case MultiMC::Succeeded:
		return 0;
	}
}
