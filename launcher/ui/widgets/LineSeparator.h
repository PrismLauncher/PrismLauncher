#pragma once
#include <QWidget>

class QStyleOption;

class LineSeparator : public QWidget {
    Q_OBJECT

   public:
    /// Create a line separator. orientation is the orientation of the line.
    explicit LineSeparator(QWidget* parent, Qt::Orientation orientation = Qt::Horizontal);
    QSize sizeHint() const;
    void paintEvent(QPaintEvent*);
    void initStyleOption(QStyleOption* option) const;

   private:
    Qt::Orientation m_orientation = Qt::Horizontal;
};
