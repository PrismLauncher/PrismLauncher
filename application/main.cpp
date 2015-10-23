#include "MultiMC.h"
#include "MainWindow.h"
#include "LaunchInteraction.h"
#include <InstanceList.h>
#include <QDebug>

int launchMainWindow(MultiMC &app)
{
	MainWindow mainWin;
	mainWin.restoreState(QByteArray::fromBase64(MMC->settings()->get("MainWindowState").toByteArray()));
	mainWin.restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("MainWindowGeometry").toByteArray()));
	mainWin.show();
	mainWin.checkSetDefaultJava();
	mainWin.checkInstancePathForProblems();
	return app.exec();
}

int launchInstance(MultiMC &app, InstancePtr inst)
{
	app.minecraftlist();
	LaunchController launchController;
	launchController.setInstance(inst);
	launchController.setOnline(true);
	QMetaObject::invokeMethod(&launchController, "start", Qt::QueuedConnection);
	app.connect(&launchController, &Task::finished, [&app]()
	{
		app.quit();
	});
	return app.exec();
}

int main_gui(MultiMC &app)
{
	app.setIconTheme(MMC->settings()->get("IconTheme").toString());
	// show main window
	auto inst = app.instances()->getInstanceById(app.launchId);
	if(inst)
	{
		return launchInstance(app, inst);
	}
	return launchMainWindow(app);
}

int main(int argc, char *argv[])
{
	// initialize Qt
	MultiMC app(argc, argv);

	Q_INIT_RESOURCE(instances);
	Q_INIT_RESOURCE(multimc);
	Q_INIT_RESOURCE(backgrounds);
	Q_INIT_RESOURCE(versions);

	Q_INIT_RESOURCE(pe_dark);
	Q_INIT_RESOURCE(pe_light);
	Q_INIT_RESOURCE(pe_blue);
	Q_INIT_RESOURCE(pe_colored);
	Q_INIT_RESOURCE(OSX);
	Q_INIT_RESOURCE(iOS);

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
