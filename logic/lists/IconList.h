#pragma once

#include <QMutex>
#include <QAbstractListModel>
#include <QtGui/QIcon>

class Private;

class IconList : public QAbstractListModel
{
public:
	IconList();
	virtual ~IconList();
	
	QIcon getIcon ( QString key );
	int getIconIndex ( QString key );
	
	virtual QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
	virtual int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
	
	bool addIcon(QString key, QString name, QString path, bool is_builtin = false);
	bool deleteIcon(QString key);
	
	virtual QStringList mimeTypes() const;
	virtual Qt::DropActions supportedDropActions() const;
	virtual bool dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );
	virtual Qt::ItemFlags flags ( const QModelIndex& index ) const;
	
	void installIcons ( QStringList iconFiles );
	
private:
	// hide copy constructor
	IconList ( const IconList & ) = delete;
	// hide assign op
	IconList& operator= ( const IconList & ) = delete;
	void reindex();
	Private* d;
};
