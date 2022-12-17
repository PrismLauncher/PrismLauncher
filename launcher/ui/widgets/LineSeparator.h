#pragma once
#include <QWidget>

class QStyleOption;

class LineSeparator : public QWidget
{
    Q_OBJECT

public:
    /// Create a line separator. orientation is the orientation of the line.
    explicit LineSeparator(QWidget *parent, Qt::Orientation orientation = Qt::Horizontal);
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *);
    void initStyleOption(QStyleOption *option) const;
private:
    Qt::Orientation hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_orientation = Qt::Horizontal;
};
