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
    QSize hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_size;
    QIcon hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icon;
};
