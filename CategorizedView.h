#ifndef WIDGET_H
#define WIDGET_H

#include <QListView>
#include <QLineEdit>
#include <QCache>

class CategorizedView : public QListView
{
	Q_OBJECT

public:
	CategorizedView(QWidget *parent = 0);
	~CategorizedView();

	enum
	{
		CategoryRole = Qt::UserRole
	};

	virtual QRect visualRect(const QModelIndex &index) const;
	QModelIndex indexAt(const QPoint &point) const;
	void setSelection(const QRect &rect, const QItemSelectionModel::SelectionFlags commands) override;

protected slots:
	 void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
	virtual void rowsInserted(const QModelIndex &parent, int start, int end);
	virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
	virtual void updateGeometries();

protected:
	virtual bool isIndexHidden(const QModelIndex &index) const;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	void startDrag(Qt::DropActions supportedActions) override;

private:
	struct Category
	{
		Category(const QString &text, CategorizedView *view);
		Category(const Category *other);
		CategorizedView *view;
		QString text;
		bool collapsed;
		QRect iconRect;
		QRect textRect;

		void drawHeader(QPainter *painter, const int y);
		int totalHeight() const;
		int headerHeight() const;
		int contentHeight() const;
		QSize categoryTotalSize() const;
	};
	friend struct Category;

	QList<Category *> m_categories;
	mutable QCache<const Category *, QList<QModelIndex> > m_cachedCategoryToIndexMapping;
	mutable QCache<const QModelIndex, QRect> m_cachedVisualRects;

	int m_leftMargin;
	int m_rightMargin;
	int m_categoryMargin;
	int m_itemSpacing;

	//bool m_updatesDisabled;

	Category *category(const QModelIndex &index) const;
	Category *category(const QString &cat) const;
	Category *categoryAt(const QPoint &pos) const;
	int numItemsForCategory(const Category *category) const;
	QList<QModelIndex> itemsForCategory(const Category *category) const;
	QModelIndex firstItemForCategory(const Category *category) const;
	QModelIndex lastItemForCategory(const Category *category) const;

	int categoryTop(const Category *category) const;

	int itemsPerRow() const;
	int contentWidth() const;

	static bool lessThanCategoryPointer(const Category *c1, const Category *c2);
	QList<Category *> sortedCategories() const;

private:
	mutable int m_cachedItemWidth;
	mutable QCache<QModelIndex, QSize> m_cachedItemSizes;
	int itemWidth() const;
	QSize itemSize(const QModelIndex &index) const;

	/*QLineEdit *m_categoryEditor;
	Category *m_editedCategory;
	void startCategoryEditor(Category *category);

private slots:
	void endCategoryEditor();*/

private:
	QPoint m_pressedPosition;
	QPersistentModelIndex m_pressedIndex;
	bool m_pressedAlreadySelected;
	Category *m_pressedCategory;
	QItemSelectionModel::SelectionFlag m_ctrlDragSelectionFlag;
	QPoint m_lastDragPosition;

	QPair<int, int> categoryInternalPosition(const QModelIndex &index) const;

	QPixmap renderToPixmap(const QModelIndexList &indices, QRect *r) const;
	QList<QPair<QRect, QModelIndex> > draggablePaintPairs(const QModelIndexList &indices, QRect *r) const;

	bool isDragEventAccepted(QDropEvent *event);

	QPair<Category *, int> rowDropPos(const QPoint &pos);
};

#endif // WIDGET_H
