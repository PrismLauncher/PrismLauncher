#ifndef WIDGET_H
#define WIDGET_H

#include <QListView>
#include <QLineEdit>

struct CategorizedViewRoles
{
	enum
	{
		CategoryRole = Qt::UserRole,
		ProgressValueRole,
		ProgressMaximumRole
	};
};

struct CategorizedViewCategory;

class CategorizedView : public QListView
{
	Q_OBJECT

public:
	CategorizedView(QWidget *parent = 0);
	~CategorizedView();

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
	friend struct CategorizedViewCategory;

	QList<CategorizedViewCategory *> m_categories;

	int m_leftMargin;
	int m_rightMargin;
	int m_bottomMargin;
	int m_categoryMargin;

	//bool m_updatesDisabled;

	CategorizedViewCategory *category(const QModelIndex &index) const;
	CategorizedViewCategory *category(const QString &cat) const;
	CategorizedViewCategory *categoryAt(const QPoint &pos) const;

	int itemsPerRow() const;
	int contentWidth() const;

private:
	int itemWidth() const;
	int categoryRowHeight(const QModelIndex &index) const;

	/*QLineEdit *m_categoryEditor;
	Category *m_editedCategory;
	void startCategoryEditor(Category *category);

private slots:
	void endCategoryEditor();*/

private:
	QPoint m_pressedPosition;
	QPersistentModelIndex m_pressedIndex;
	bool m_pressedAlreadySelected;
	CategorizedViewCategory *m_pressedCategory;
	QItemSelectionModel::SelectionFlag m_ctrlDragSelectionFlag;
	QPoint m_lastDragPosition;

	QPair<int, int> categoryInternalPosition(const QModelIndex &index) const;
	int categoryInternalRowTop(const QModelIndex &index) const;
	int itemHeightForCategoryRow(const CategorizedViewCategory *category, const int internalRow) const;

	QPixmap renderToPixmap(const QModelIndexList &indices, QRect *r) const;
	QList<QPair<QRect, QModelIndex> > draggablePaintPairs(const QModelIndexList &indices, QRect *r) const;

	bool isDragEventAccepted(QDropEvent *event);

	QPair<CategorizedViewCategory *, int> rowDropPos(const QPoint &pos);

	QPoint offset() const;
};

#endif // WIDGET_H
