#include "IconLabel.h"

#include <QStyle>
#include <QStyleOption>
#include <QLayout>
#include <QPainter>
#include <QRect>

IconLabel::IconLabel(QWidget *parent, QIcon icon, QSize size)
    : QWidget(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_size(size), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icon(icon)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

QSize IconLabel::sizeHint() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_size;
}

void IconLabel::setIcon(QIcon icon)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icon = icon;
    update();
}

void IconLabel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect rect = contentsRect();
    int width = rect.width();
    int height = rect.height();
    if(width < height)
    {
        rect.setHeight(width);
        rect.translate(0, (height - width) / 2);
    }
    else if (width > height)
    {
        rect.setWidth(height);
        rect.translate((width - height) / 2, 0);
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icon.paint(&p, rect);
}
