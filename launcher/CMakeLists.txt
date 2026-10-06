#pragma once

#include <QPushButton>

#include "ui/designsystem/DesignTokens.h"

class ModernButton : public QPushButton {
    Q_OBJECT
public:
    enum class Variant {
        Primary,
        Secondary,
        Ghost
    };

    explicit ModernButton(QWidget* parent = nullptr) : QPushButton(parent), m_variant(Variant::Primary)
    {
        setObjectName("ModernButton");
        setVariant(m_variant);
    }

    void setVariant(Variant variant)
    {
        m_variant = variant;
        switch (m_variant) {
            case Variant::Primary:
                setStyleSheet(QString(
                    "QPushButton { background-color: %1; color: %2; border: none; border-radius: %3px; padding: 8px 14px; }"
                    "QPushButton:hover { background-color: %4; }"
                    "QPushButton:pressed { background-color: %5; }"
                    .arg(DesignTokens::Palette::Primary.name())
                    .arg(DesignTokens::Palette::Text.name())
                    .arg(DesignTokens::Metrics::RadiusMedium)
                    .arg(DesignTokens::Palette::PrimaryHover.name())
                    .arg(DesignTokens::Palette::Primary.name())));
                break;
            case Variant::Secondary:
                setStyleSheet(QString(
                    "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 8px 14px; }"
                    "QPushButton:hover { background-color: %5; }"
                    .arg(DesignTokens::Palette::Elevated.name())
                    .arg(DesignTokens::Palette::Text.name())
                    .arg(DesignTokens::Palette::Divider.name())
                    .arg(DesignTokens::Metrics::RadiusMedium)
                    .arg(DesignTokens::Palette::Surface.name())));
                break;
            case Variant::Ghost:
                setStyleSheet(QString(
                    "QPushButton { background: transparent; color: %1; border: 1px solid transparent; border-radius: %2px; padding: 8px 14px; }"
                    "QPushButton:hover { background-color: %3; }"
                    .arg(DesignTokens::Palette::SecondaryText.name())
                    .arg(DesignTokens::Metrics::RadiusMedium)
                    .arg(DesignTokens::Palette::Elevated.name())));
                break;
        }
    }

private:
    Variant m_variant;
};
