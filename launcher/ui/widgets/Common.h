#pragma once

#include <QTextLayout>

/** Cuts out the text in textLayout into smaller pieces, according to the lineWidth.
 *  Returns a list of pairs, each containing the width of that line and that line's string, respectively.
 *  The total height of those lines is set in the last argument, 'height'.
 */
QList<std::pair<qreal, QString>> viewItemTextLayout(QTextLayout& textLayout, int lineWidth, qreal& height);
