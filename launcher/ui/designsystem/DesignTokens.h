#pragma once

#include <QColor>
#include <QSize>

namespace DesignTokens {
namespace Palette {
    inline constexpr auto Background = QColor(0x0f, 0x11, 0x17);
    inline constexpr auto Surface = QColor(0x17, 0x1b, 0x22);
    inline constexpr auto Elevated = QColor(0x1f, 0x24, 0x30);
    inline constexpr auto Divider = QColor(0x2c, 0x33, 0x40);
    inline constexpr auto Primary = QColor(0x7c, 0x3a, 0xed);
    inline constexpr auto PrimaryHover = QColor(0x8b, 0x5c, 0xf6);
    inline constexpr auto Text = QColor(0xf5, 0xf7, 0xfb);
    inline constexpr auto SecondaryText = QColor(0xa7, 0xb0, 0xc2);
    inline constexpr auto Success = QColor(0x22, 0xc5, 0x5e);
    inline constexpr auto Warning = QColor(0xf5, 0x9e, 0x0b);
    inline constexpr auto Danger = QColor(0xf8, 0x71, 0x71);
}

namespace Metrics {
    inline constexpr int RadiusSmall = 8;
    inline constexpr int RadiusMedium = 12;
    inline constexpr int RadiusLarge = 18;
    inline constexpr int SpacingXS = 4;
    inline constexpr int SpacingSM = 8;
    inline constexpr int SpacingMD = 12;
    inline constexpr int SpacingLG = 16;
    inline constexpr int SpacingXL = 20;
}
}
