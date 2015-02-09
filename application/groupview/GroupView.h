#pragma once

#include <QListView>
#include <QLineEdit>
#include <QScrollBar>
#include <QCache>

struct GroupViewRoles
{
	enum
	{
		GroupRole = Qt::UserRole,
		ProgressValueRole,
		ProgressMaximumRole
	};
};

struct VisualGroup;

class GroupView : public QAbstractItemView
{
	Q_OBJECT

public:
	GroupView(QWidget *parent = 0);
	~GroupView();

	void setModel(QAbstractItemModel *model) override;

	/// return geometry rectangle occupied by the specified model item
	QRect geometryRect(const QModelIndex &index) const;
	/// return visual rectangle occupied by the specified model item
	virtual QRect visualRect(const QModelIndex &index) const override;
	/// get the model index at the specified visual point
	virtual QModelIndex indexAt(const QPoint &point) const override;
	void setSelection(const QRect &rect,
					  const QItemSelectionModel::SelectionFlags commands) override;

	virtual int horizontalOffset() const override;
	virtual int verticalOffset() const override;
	virtual void scrollContentsBy(int dx, int dy) override;
	virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;

	virtual QModelIndex moveCursor(CursorAction cursorAction,
								   Qt::KeyboardModifiers modifiers) override;

	virtual QRegion visualRegionForSelection(const QItemSelection &selection) const override;

	int spacing() const
	{
		return m_spacing;
	};
protected
slots:
	virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
							 const QVector<int> &roles) override;
	virtual void rowsInserted(const QModelIndex &parent, int start, int end) override;
	virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;
	virtual void updateGeometries() override;
	void modelReset();

protected:
	virtual bool isIndexHidden(const QModelIndex &index) const override;
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
	friend struct VisualGroup;
	QList<VisualGroup *> m_groups;

	// geometry
	int m_leftMargin = 5;
	int m_rightMargin = 5;
	int m_bottomMargin = 5;
	int m_categoryMargin = 5;
	int m_spacing = 5;
	int m_itemWidth = 100;
	int m_currentItemsPerRow = -1;
	int m_currentCursorColumn= -1;
	mutable QCache<int, QRect> geometryCache;

	// point where the currently active mouse action started in geometry coordinates
	QPoint m_pressedPosition;
	QPersistentModelIndex m_pressedIndex;
	bool m_pressedAlreadySelected;
	VisualGroup *m_pressedCategory;
	QItemSelectionModel::SelectionFlag m_ctrlDragSelectionFlag;
	QPoint m_lastDragPosition;

	VisualGroup *category(const QModelIndex &index) const;
	VisualGroup *category(const QString &cat) const;
	VisualGroup *categoryAt(const QPoint &pos) const;

	int itemsPerRow() const
	{
		return m_currentItemsPerRow;
	};
	int contentWidth() const;

private: /* methods */
	int itemWidth() const;
	int calculateItemsPerRow() const;
	int verticalScrollToValue(const QModelIndex &index, const QRect &rect,
							  QListView::ScrollHint hint) const;
	QPixmap renderToPixmap(const QModelIndexList &indices, QRect *r) const;
	QList<QPair<QRect, QModelIndex>> draggablePaintPairs(const QModelIndexList &indices,
														 QRect *r) const;

	bool isDragEventAccepted(QDropEvent *event);

	QPair<VisualGroup *, int> rowDropPos(const QPoint &pos);

	QPoint offset() const;
};
