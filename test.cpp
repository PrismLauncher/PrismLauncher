
#include <iostream>

#include "keyring.h"
#include "cmdutils.h"

using namespace Util::Commandline;

#include <QCoreApplication>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	app.setApplicationName("MMC Keyring test");
	app.setOrganizationName("Orochimarufan");

	Parser p;
	p.addArgument("user", false);
	p.addArgument("password", false);
	p.addSwitch("set");
	p.addSwitch("get");
	p.addSwitch("list");
	p.addOption("service", "Test");
	p.addShortOpt("service", 's');

	QHash<QString, QVariant> args;
	try {
		args = p.parse(app.arguments());
	} catch (ParsingError) {
		std::cout << "Syntax error." << std::endl;
		return 1;
	}

	if (args["set"].toBool()) {
		if (args["user"].isNull() || args["password"].isNull()) {
			std::cout << "set operation needs bot user and password set" << std::endl;
			return 1;
		}

		return Keyring::instance()->storePassword(args["service"].toString(),
				args["user"].toString(), args["password"].toString());
	} else if (args["get"].toBool()) {
		if (args["user"].isNull()) {
			std::cout << "get operation needs user set" << std::endl;
			return 1;
		}

		std::cout << "Password: " << qPrintable(Keyring::instance()->getPassword(args["service"].toString(),
				args["user"].toString())) << std::endl;
		return 0;
	} else if (args["list"].toBool()) {
		QStringList accounts = Keyring::instance()->getStoredAccounts(args["service"].toString());
		std::cout << "stored accounts:" << std::endl << '\t' << qPrintable(accounts.join("\n\t")) << std::endl;
		return 0;
	} else {
		std::cout << "No operation given!" << std::endl;
		std::cout << qPrintable(p.compileHelp(argv[0])) << std::endl;
		return 1;
	}
}
