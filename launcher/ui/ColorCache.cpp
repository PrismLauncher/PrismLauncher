#include "ColorCache.h"


/**
 * Blend the color with the front color, adapting to the back color
 */
QColor ColorCache::blend(QColor color)
{
    if (Rainbow::luma(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front) > Rainbow::luma(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_back))
    {
        // for dark color schemes, produce a fitting color first
        color = Rainbow::tint(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front, color, 0.5);
    }
    // adapt contrast
    return Rainbow::mix(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_front, color, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bias);
}

/**
 * Blend the color with the back color
 */
QColor ColorCache::blendBackground(QColor color)
{
    // adapt contrast
    return Rainbow::mix(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_back, color, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bias);
}

void ColorCache::recolorAll()
{
    auto iter = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.begin();
    while(iter != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_colors.end())
    {
        iter->front = blend(iter->original);
        iter->back = blendBackground(iter->original);
    }
}
