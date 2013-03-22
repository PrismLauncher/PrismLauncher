#pragma once

#include <QMutex>
#include <QtGui/QIcon>

class Private;

class IconCache
{
public:
	static IconCache* instance()
	{
		if (!m_Instance)
		{
			mutex.lock();
			if (!m_Instance)
				m_Instance = new IconCache;
			mutex.unlock();
		}
		return m_Instance;
	}

	static void drop()
	{
		mutex.lock();
		delete m_Instance;
		m_Instance = 0;
		mutex.unlock();
	}

	QIcon getIcon(QString name);
	
private:
	IconCache();
	// hide copy constructor
	IconCache(const IconCache &);
	// hide assign op
	IconCache& operator=(const IconCache &); 
	static IconCache* m_Instance;
	static QMutex mutex;
	Private* d;
};
	