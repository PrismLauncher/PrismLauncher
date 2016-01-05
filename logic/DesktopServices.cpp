#include "DesktopServices.h"
#include <QDir>
#include <QDesktopServices>
#include <QProcess>
#include <QDebug>

/**
 * This shouldn't exist, but until QTBUG-9328 and other unreported bugs are fixed, it needs to be a thing.
 */
#if defined(Q_OS_LINUX)

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

template <typename T>
bool IndirectOpen(T callable, qint64 *pid_forked = nullptr)
{
	auto pid = fork();
	if(pid_forked)
	{
		if(pid > 0)
			*pid_forked = pid;
		else
			*pid_forked = 0;
	}
	if(pid == -1)
	{
		qWarning() << "IndirectOpen failed to fork: " << errno;
		return false;
	}
	// child - do the stuff
	if(pid == 0)
	{
		// unset all this garbage so it doesn't get passed to the child process
		qunsetenv("LD_PRELOAD");
		qunsetenv("LD_LIBRARY_PATH");
		qunsetenv("LD_DEBUG");
		qunsetenv("QT_PLUGIN_PATH");
		qunsetenv("QT_FONTPATH");

		// open the URL
		auto status = callable();

		// detach from the parent process group.
		setsid();

		// die. now. do not clean up anything, it would just hang forever.
		_exit(status ? 0 : 1);
	}
	else
	{
		//parent - assume it worked.
		int status;
		while (waitpid(pid, &status, 0))
		{
			if(WIFEXITED(status))
			{
				return WEXITSTATUS(status) == 0;
			}
			if(WIFSIGNALED(status))
			{
				return false;
			}
		}
		return true;
	}
}
#endif

namespace DesktopServices {
bool openDirectory(const QString &path, bool ensureExists)
{
	qDebug() << "Opening directory" << path;
	QDir parentPath;
	QDir dir(path);
	if (!dir.exists())
	{
		parentPath.mkpath(dir.absolutePath());
	}
	auto f = [&]()
	{
		return QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
	};
#if defined(Q_OS_LINUX)
	return IndirectOpen(f);
#else
	return f();
#endif
}

bool openFile(const QString &path)
{
	qDebug() << "Opening file" << path;
	auto f = [&]()
	{
		return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	};
#if defined(Q_OS_LINUX)
	return IndirectOpen(f);
#else
	return f();
#endif
}

bool openFile(const QString &application, const QString &path, const QString &workingDirectory, qint64 *pid)
{
	qDebug() << "Opening file" << path << "using" << application;
#if defined(Q_OS_LINUX)
	// FIXME: the pid here is fake. So if something depends on it, it will likely misbehave
	return IndirectOpen([&]()
	{
		return QProcess::startDetached(application, QStringList() << path, workingDirectory);
	}, pid);
#else
	return QProcess::startDetached(application, QStringList() << path, workingDirectory, pid);
#endif
}

bool run(const QString &application, const QStringList &args, const QString &workingDirectory, qint64 *pid)
{
	qDebug() << "Running" << application << "with args" << args.join(' ');
#if defined(Q_OS_LINUX)
	// FIXME: the pid here is fake. So if something depends on it, it will likely misbehave
	return IndirectOpen([&]()
	{
		return QProcess::startDetached(application, args, workingDirectory);
	}, pid);
#else
	return QProcess::startDetached(application, args, workingDirectory, pid);
#endif
}

bool openUrl(const QUrl &url)
{
	qDebug() << "Opening URL" << url.toString();
	auto f = [&]()
	{
		return QDesktopServices::openUrl(url);
	};
#if defined(Q_OS_LINUX)
	return IndirectOpen(f);
#else
	return f();
#endif
}

}
