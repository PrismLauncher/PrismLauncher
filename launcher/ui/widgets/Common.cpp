#include "Common.h"

// Origin: Qt
// More specifically, this is a trimmed down version on the algorithm in:
// https://code.woboq.org/qt5/qtbase/src/widgets/styles/qcommonstyle.cpp.html#846
QList<std::pair<qreal, QString>> viewItemTextLayout(QTextLayout& textLayout, int lineWidth, qreal& height)
{
    QList<std::pair<qreal, QString>> lines;
    height = 0;

    textLayout.beginLayout();

    QString str = textLayout.text();
    while (true) {
        QTextLine line = textLayout.createLine();

        if (!line.isValid())
            break;
        if (line.textLength() == 0)
            break;

        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));

        height += line.height();

        lines.append(std::make_pair(line.naturalTextWidth(), str.mid(line.textStart(), line.textLength())));
    }

    textLayout.endLayout();

    return lines;
}
