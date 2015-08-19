#pragma once
#include <QtGui/QColor>
#include <rainbow.h>
namespace Color
{
/**
 * Blend the color with the front color, adapting to the back color
 */
QColor blend(QColor front, QColor back, QColor color, uchar ratio);

/**
 * Blend the color with the back color
 */
QColor blendBackground(QColor back, QColor color, uchar ratio);
}
