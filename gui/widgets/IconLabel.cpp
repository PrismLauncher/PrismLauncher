#include "IconLabel.h"

#include <QStyle>
#include <QStyleOption>
#include <QLayout>
#include <QPainter>
#include <QRect>

IconLabel::IconLabel(QWidget *parent, QIcon icon, QSize size)
	: QWidget(parent), m_size(size), m_icon(icon)
{
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

QSize IconLabel::sizeHint() const
{
	return m_size;
}

void IconLabel::setIcon(QIcon icon)
{
	m_icon = icon;
	update();
}

void IconLabel::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	m_icon.paint(&p, contentsRect());
}
