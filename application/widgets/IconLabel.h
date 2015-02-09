#pragma once
#include <QWidget>
#include <QIcon>

class QStyleOption;

/**
 * This is a trivial widget that paints a QIcon of the specified size.
 */
class IconLabel : public QWidget
{
	Q_OBJECT

public:
	/// Create a line separator. orientation is the orientation of the line.
	explicit IconLabel(QWidget *parent, QIcon icon, QSize size);

	virtual QSize sizeHint() const;
	virtual void paintEvent(QPaintEvent *);

	void setIcon(QIcon icon);

private:
	QSize m_size;
	QIcon m_icon;
};
