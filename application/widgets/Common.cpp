#include "Common.h"

// Origin: Qt
QStringList viewItemTextLayout(QTextLayout &textLayout, int lineWidth, qreal &height,
                               qreal &widthUsed)
{
    QStringList lines;
    height = 0;
    widthUsed = 0;
    textLayout.beginLayout();
    QString str = textLayout.text();
    while (true)
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        if (line.textLength() == 0)
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        lines.append(str.mid(line.textStart(), line.textLength()));
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return lines;
}
