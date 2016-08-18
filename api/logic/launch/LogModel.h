#pragma once

#include <QAbstractListModel>
#include <QString>
#include "MessageLevel.h"

#include <multimc_logic_export.h>

class MULTIMC_LOGIC_EXPORT LogModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit LogModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;

	void append(MessageLevel::Enum, QString line);
	void clear();

	QString toPlainText();

	void setMaxLines(int maxLines);
	void setStopOnOverflow(bool stop);
	void setOverflowMessage(const QString & overflowMessage);

	enum Roles
	{
		LevelRole = Qt::UserRole
	};

private /* types */:
	struct entry
	{
		MessageLevel::Enum level;
		QString line;
	};

private: /* data */
	QVector <entry> m_content;
	int m_maxLines = 1000;
	// first line in the circular buffer
	int m_firstLine = 0;
	// number of lines occupied in the circular buffer
	int m_numLines = 0;
	bool m_stopOnOverflow = false;
	QString m_overflowMessage = "OVERFLOW";

private:
	Q_DISABLE_COPY(LogModel)
};
