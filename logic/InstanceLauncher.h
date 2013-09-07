#pragma once

#include <QObject>

class MinecraftProcess;
class ConsoleWindow;

// Commandline instance launcher
class InstanceLauncher : public QObject
{
	Q_OBJECT
	
private:
	QString instId;
	MinecraftProcess *proc;
	ConsoleWindow *console;
	
public:
	InstanceLauncher(QString instId);
	
private slots:
	void onTerminated();
	void onLoginComplete();
	void doLogin(const QString &errorMsg);
	
public:
	int launch();
};
