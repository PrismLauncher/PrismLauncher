#include "Colors.h"

/**
 * Blend the color with the front color, adapting to the back color
 */
QColor Color::blend(QColor front, QColor back, QColor color, uchar ratio)
{
	Q_ASSERT(front.isValid());
	Q_ASSERT(back.isValid());
	if (Rainbow::luma(front) > Rainbow::luma(back))
	{
		// for dark color schemes, produce a fitting color first
		color = Rainbow::tint(front, color, 0.5);
	}
	// adapt contrast
	return Rainbow::mix(front, color, float(ratio) / float(0xff));
}

/**
 * Blend the color with the back color
 */
QColor Color::blendBackground(QColor back, QColor color, uchar ratio)
{
	// adapt contrast
	return Rainbow::mix(back, color, float(ratio) / float(0xff));
}
