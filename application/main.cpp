#include "MultiMC.h"
#include "MainWindow.h"
#include "LaunchController.h"
#include <InstanceList.h>
#include <QDebug>

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
		return app.exec();
	case MultiMC::Failed:
		return 1;
	case MultiMC::Succeeded:
		return 0;
	}
}
