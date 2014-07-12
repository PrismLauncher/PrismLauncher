#include "RecursiveFileSystemWatcher.h"

#include <QRegularExpression>

RecursiveFileSystemWatcher::RecursiveFileSystemWatcher(QObject *parent)
	: QObject(parent),
	  m_watcher(new QFileSystemWatcher(this))
{
	connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &RecursiveFileSystemWatcher::fileChange);
	connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &RecursiveFileSystemWatcher::directoryChange);
}

void RecursiveFileSystemWatcher::setRootDir(const QDir &root)
{
	bool wasEnabled = m_isEnabled;
	disable();
	m_root = root;
	setFiles(scanRecursive(m_root));
	if (wasEnabled)
	{
		enable();
	}
}
void RecursiveFileSystemWatcher::setWatchFiles(const bool watchFiles)
{
	bool wasEnabled = m_isEnabled;
	disable();
	m_watchFiles = watchFiles;
	if (wasEnabled)
	{
		enable();
	}
}

void RecursiveFileSystemWatcher::enable()
{
	Q_ASSERT(m_root != QDir::root());
	addFilesToWatcherRecursive(m_root);
	m_isEnabled = true;
}
void RecursiveFileSystemWatcher::disable()
{
	m_isEnabled = false;
	m_watcher->removePaths(m_watcher->files());
	m_watcher->removePaths(m_watcher->directories());
}

void RecursiveFileSystemWatcher::setFiles(const QStringList &files)
{
	if (files != m_files)
	{
		m_files = files;
		emit filesChanged();
	}
}

void RecursiveFileSystemWatcher::addFilesToWatcherRecursive(const QDir &dir)
{
	m_watcher->addPath(dir.absolutePath());
	for (const QFileInfo &info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		addFilesToWatcherRecursive(info.absoluteDir());
	}
	if (m_watchFiles)
	{
		for (const QFileInfo &info : dir.entryInfoList(QDir::Files))
		{
			m_watcher->addPath(info.absoluteFilePath());
		}
	}
}
QStringList RecursiveFileSystemWatcher::scanRecursive(const QDir &dir)
{
	QStringList ret;
	QRegularExpression exp(m_exp);
	for (const QFileInfo &info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files))
	{
		if (info.isFile() && exp.match(info.absoluteFilePath()).hasMatch())
		{
			ret.append(info.absoluteFilePath());
		}
		else if (info.isDir())
		{
			ret.append(scanRecursive(info.absoluteDir()));
		}
	}
	return ret;
}

void RecursiveFileSystemWatcher::fileChange(const QString &path)
{
	emit fileChanged(path);
}
void RecursiveFileSystemWatcher::directoryChange(const QString &path)
{
	setFiles(scanRecursive(m_root));
}
