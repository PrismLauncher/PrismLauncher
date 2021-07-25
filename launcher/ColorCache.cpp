#include "ColorCache.h"


/**
 * Blend the color with the front color, adapting to the back color
 */
QColor ColorCache::blend(QColor color)
{
    if (Rainbow::luma(m_front) > Rainbow::luma(m_back))
    {
        // for dark color schemes, produce a fitting color first
        color = Rainbow::tint(m_front, color, 0.5);
    }
    // adapt contrast
    return Rainbow::mix(m_front, color, m_bias);
}

/**
 * Blend the color with the back color
 */
QColor ColorCache::blendBackground(QColor color)
{
    // adapt contrast
    return Rainbow::mix(m_back, color, m_bias);
}

void ColorCache::recolorAll()
{
    auto iter = m_colors.begin();
    while(iter != m_colors.end())
    {
        iter->front = blend(iter->original);
        iter->back = blendBackground(iter->original);
    }
}
