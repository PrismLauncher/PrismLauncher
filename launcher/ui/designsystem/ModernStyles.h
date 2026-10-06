#pragma once

#include <QString>

#include "ui/designsystem/DesignTokens.h"

namespace ModernStyles {
inline QString cardStyle()
{
    return QString(
        "QWidget { background-color: %1; border: 1px solid %2; border-radius: %3px; color: %4; }"
            .arg(DesignTokens::Palette::Surface.name())
            .arg(DesignTokens::Palette::Divider.name())
            .arg(DesignTokens::Metrics::RadiusMedium)
            .arg(DesignTokens::Palette::Text.name()));
}

inline QString primaryButtonStyle()
{
    return QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: %3px; padding: 8px 14px; }"
            "QPushButton:hover { background-color: %4; }"
            "QPushButton:pressed { background-color: %5; }"
            .arg(DesignTokens::Palette::Primary.name())
            .arg(DesignTokens::Palette::Text.name())
            .arg(DesignTokens::Metrics::RadiusMedium)
            .arg(DesignTokens::Palette::PrimaryHover.name())
            .arg(DesignTokens::Palette::Primary.name()));
}

inline QString secondaryButtonStyle()
{
    return QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 8px 14px; }"
            "QPushButton:hover { background-color: %5; }"
            .arg(DesignTokens::Palette::Elevated.name())
            .arg(DesignTokens::Palette::Text.name())
            .arg(DesignTokens::Palette::Divider.name())
            .arg(DesignTokens::Metrics::RadiusMedium)
            .arg(DesignTokens::Palette::Surface.name()));
}
}
