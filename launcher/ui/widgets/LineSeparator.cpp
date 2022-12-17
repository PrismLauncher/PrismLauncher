#include "LineSeparator.h"

#include <QStyle>
#include <QStyleOption>
#include <QLayout>
#include <QPainter>

void LineSeparator::initStyleOption(QStyleOption *option) const
{
    option->initFrom(this);
    // in a horizontal layout, the line is vertical (and vice versa)
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_orientation == Qt::Vertical)
        option->state |= QStyle::State_Horizontal;
}

LineSeparator::LineSeparator(QWidget *parent, Qt::Orientation orientation)
    : QWidget(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_orientation(orientation)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

QSize LineSeparator::sizeHint() const
{
    QStyleOption opt;
    initStyleOption(&opt);
    const int extent =
        style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, &opt, parentWidget());
    return QSize(extent, extent);
}

void LineSeparator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt;
    initStyleOption(&opt);
    style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, parentWidget());
}
