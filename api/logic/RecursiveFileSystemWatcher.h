#pragma once

#include <QFileSystemWatcher>
#include <QDir>
#include "pathmatcher/IPathMatcher.h"

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT RecursiveFileSystemWatcher : public QObject
{
	Q_OBJECT
public:
	RecursiveFileSystemWatcher(QObject *parent);

	void setRootDir(const QDir &root);
	QDir rootDir() const
	{
		return m_root;
	}

	// WARNING: setting this to true may be bad for performance
	void setWatchFiles(const bool watchFiles);
	bool watchFiles() const
	{
		return m_watchFiles;
	}

	void setMatcher(IPathMatcher::Ptr matcher)
	{
		m_matcher = matcher;
	}

	QStringList files() const
	{
		return m_files;
	}

signals:
	void filesChanged();
	void fileChanged(const QString &path);

public slots:
	void enable();
	void disable();

private:
	QDir m_root;
	bool m_watchFiles = false;
	bool m_isEnabled = false;
	IPathMatcher::Ptr m_matcher;

	QFileSystemWatcher *m_watcher;

	QStringList m_files;
	void setFiles(const QStringList &files);

	void addFilesToWatcherRecursive(const QDir &dir);
	QStringList scanRecursive(const QDir &dir);

private slots:
	void fileChange(const QString &path);
	void directoryChange(const QString &path);
};
