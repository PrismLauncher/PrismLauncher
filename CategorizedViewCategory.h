#ifndef CATEGORIZEDVIEWROW_H
#define CATEGORIZEDVIEWROW_H

#include <QString>
#include <QRect>
#include <QVector>

class CategorizedView;
class QPainter;
class QModelIndex;

struct CategorizedViewCategory
{
	CategorizedViewCategory(const QString &text, CategorizedView *view);
	CategorizedViewCategory(const CategorizedViewCategory *other);
	CategorizedView *view;
	QString text;
	bool collapsed;
	QRect iconRect;
	QRect textRect;
	QVector<int> rowHeights;
	int firstRow;

	void update();

	void drawHeader(QPainter *painter, const int y);
	int totalHeight() const;
	int headerHeight() const;
	int contentHeight() const;
	int numRows() const;
	int top() const;

	QList<QModelIndex> items() const;
	int numItems() const;
	QModelIndex firstItem() const;
	QModelIndex lastItem() const;
};

#endif // CATEGORIZEDVIEWROW_H
