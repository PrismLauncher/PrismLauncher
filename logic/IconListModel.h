#pragma once

#include <QMutex>
#include <QAbstractListModel>
#include <QtGui/QIcon>

class Private;

class IconList : public QAbstractListModel
{
public:
	static IconList* instance();
	static void drop();
	QIcon getIcon ( QString key );
	int getIconIndex ( QString key );
	
	virtual QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
	virtual int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
	
	bool addIcon(QString key, QString name, QString path, bool is_builtin = false);
	
	
private:
	virtual ~IconList();
	IconList();
	// hide copy constructor
	IconList ( const IconList & ) = delete;
	// hide assign op
	IconList& operator= ( const IconList & ) = delete;
	static IconList* m_Instance;
	static QMutex mutex;
	Private* d;
};
