#pragma once
#include <QStringList>
#include <QTextLayout>

QStringList viewItemTextLayout(QTextLayout &textLayout, int lineWidth, qreal &height,
						qreal &widthUsed);