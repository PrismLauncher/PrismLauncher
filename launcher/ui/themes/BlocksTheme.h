// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "BlocksStyle.h"
#include "FusionTheme.h"

// Minecraft-flavoured theme: stone buttons, inventory slots for instances, a dirt backdrop and a pixel display face.
class BlocksTheme : public FusionTheme {
   public:
    enum class Variant { Inventory, Deepslate };

    explicit BlocksTheme(Variant variant) : m_variant(variant) {}
    ~BlocksTheme() override = default;

    void apply(bool initial) override;
    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override { return false; }
    QString appStyleSheet() override { return {}; }
    QPalette colorScheme() override;
    double fadeAmount() override { return 0.5; }
    QColor fadeColor() override;
    LogColors logColorScheme() override;
    ThemeTokens tokens() override;
    QList<std::pair<QByteArray, QFont>> widgetFonts() override;

    // Registers the bundled pixel face on first use and returns its family name
    static QString displayFontFamily();

   private:
    BlocksStyle::Colors styleColors() const;

    Variant m_variant;
};
