// SPDX-License-Identifier: GPL-3.0-only
#include "BlocksTheme.h"

#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QObject>
#include <QStyleFactory>

#include "Application.h"
#include "ThemeManager.h"

namespace {
bool isDeepslate(BlocksTheme::Variant variant)
{
    return variant == BlocksTheme::Variant::Deepslate;
}
}  // namespace

QString BlocksTheme::displayFontFamily()
{
    static const QString s_family = [] {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/blocks/fonts/PixelifySans.ttf"));
        const auto families = QFontDatabase::applicationFontFamilies(id);
        if (families.isEmpty()) {
            themeWarningLog() << "Couldn't load the Blocks display font";
            return QString();
        }
        return families.first();
    }();
    return s_family;
}

void BlocksTheme::apply(bool)
{
    displayFontFamily();
    APPLICATION->setStyleSheet(QString());
    QApplication::setStyle(new BlocksStyle(QStyleFactory::create(qtTheme()), styleColors()));
    QApplication::setPalette(colorScheme());
    QDir::setSearchPaths("theme", searchPaths());
}

QString BlocksTheme::id()
{
    return isDeepslate(m_variant) ? QStringLiteral("blocks-deepslate") : QStringLiteral("blocks");
}

QString BlocksTheme::name()
{
    return isDeepslate(m_variant) ? QObject::tr("Blocks (Deepslate)") : QObject::tr("Blocks");
}

QString BlocksTheme::tooltip()
{
    return isDeepslate(m_variant) ? QObject::tr("Minecraft-style stone buttons and inventory slots on dark deepslate")
                                  : QObject::tr("Minecraft-style stone buttons and inventory slots on a dirt backdrop");
}

QColor BlocksTheme::fadeColor()
{
    return isDeepslate(m_variant) ? QColor(0x2c, 0x2c, 0x31) : QColor(0xc6, 0xc6, 0xc6);
}

QPalette BlocksTheme::colorScheme()
{
    QPalette palette;
    if (isDeepslate(m_variant)) {
        palette.setColor(QPalette::Window, QColor(0x2c, 0x2c, 0x31));
        palette.setColor(QPalette::WindowText, QColor(0xe3, 0xe3, 0xe8));
        palette.setColor(QPalette::Base, QColor(0x0f, 0x0f, 0x12));
        palette.setColor(QPalette::AlternateBase, QColor(0x17, 0x17, 0x1b));
        palette.setColor(QPalette::Button, QColor(0x55, 0x55, 0x5c));
        palette.setColor(QPalette::Light, QColor(0x4f, 0x4f, 0x58));
        palette.setColor(QPalette::Midlight, QColor(0x3c, 0x3c, 0x44));
        palette.setColor(QPalette::Mid, QColor(0x26, 0x26, 0x2b));
        palette.setColor(QPalette::Dark, QColor(0x14, 0x14, 0x17));
        palette.setColor(QPalette::Link, QColor(0x8a, 0xb4, 0xff));
        palette.setColor(QPalette::LinkVisited, QColor(0xc7, 0x9b, 0xff));
        palette.setColor(QPalette::PlaceholderText, QColor(0x6f, 0x6f, 0x78));
    } else {
        // Inventory grey panels; labels in the dark grey Minecraft uses for inventory titles
        palette.setColor(QPalette::Window, QColor(0xc6, 0xc6, 0xc6));
        palette.setColor(QPalette::WindowText, QColor(0x37, 0x37, 0x37));
        palette.setColor(QPalette::Base, QColor(0x10, 0x10, 0x10));
        palette.setColor(QPalette::AlternateBase, QColor(0x1a, 0x1a, 0x1a));
        palette.setColor(QPalette::Button, QColor(0x6f, 0x6f, 0x6f));
        palette.setColor(QPalette::Light, QColor(0xff, 0xff, 0xff));
        palette.setColor(QPalette::Midlight, QColor(0xdb, 0xdb, 0xdb));
        palette.setColor(QPalette::Mid, QColor(0x8b, 0x8b, 0x8b));
        palette.setColor(QPalette::Dark, QColor(0x55, 0x55, 0x55));
        palette.setColor(QPalette::Link, QColor(0x1f, 0x3f, 0xbf));
        palette.setColor(QPalette::LinkVisited, QColor(0x6f, 0x2f, 0xbf));
        palette.setColor(QPalette::PlaceholderText, QColor(0x7a, 0x7a, 0x7a));
    }
    // Shared by both variants: black text fields, stone buttons, purple tooltips and grass-green selection
    palette.setColor(QPalette::Text, QColor(0xe0, 0xe0, 0xe0));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, QColor(0xff, 0x55, 0x55));
    palette.setColor(QPalette::Shadow, Qt::black);
    palette.setColor(QPalette::ToolTipBase, QColor(0x10, 0x00, 0x10));
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Highlight, QColor(0x3f, 0x7f, 0x2a));
    palette.setColor(QPalette::HighlightedText, Qt::white);

    palette = fadeInactive(palette, fadeAmount(), fadeColor());
    // Minecraft greys out the label of a disabled button rather than fading it into the panel
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0xa0, 0xa0, 0xa0));
    return palette;
}

LogColors BlocksTheme::logColorScheme()
{
    // Minecraft's own chat colours
    LogColors colors;
    colors.foreground[MessageLevel::Launcher] = QColor(0xff, 0x55, 0xff);  // light purple
    colors.foreground[MessageLevel::Trace] = QColor(0x55, 0x55, 0x55);     // dark grey
    colors.foreground[MessageLevel::Debug] = QColor(0xaa, 0xaa, 0xaa);     // grey
    colors.foreground[MessageLevel::Info] = QColor(0xff, 0xff, 0xff);      // white
    colors.foreground[MessageLevel::Message] = QColor(0xff, 0xff, 0xff);
    colors.foreground[MessageLevel::Warning] = QColor(0xff, 0xaa, 0x00);  // gold
    colors.foreground[MessageLevel::Error] = QColor(0xff, 0x55, 0x55);    // red
    colors.foreground[MessageLevel::Fatal] = QColor(0xff, 0xff, 0xff);
    colors.background[MessageLevel::Fatal] = QColor(0xaa, 0x00, 0x00);  // dark red
    return colors;
}

ThemeTokens BlocksTheme::tokens()
{
    ThemeTokens tokens;
    tokens.tileStyle = ThemeTokens::TileStyle::Slot;
    // Wide enough for "1.21.1 · NeoForge" under the name
    tokens.tileWidth = 112;
    tokens.tileIconSize = 48;
    tokens.tileSpacing = 8;
    tokens.tileMetaLine = true;
    tokens.pixelUnit = 2;
    tokens.selectionFrame = Qt::white;
    tokens.slotHover = QColor(255, 255, 255, 80);
    tokens.viewText = Qt::white;
    tokens.viewTextShadow = QColor(0x3f, 0x3f, 0x3f);
    tokens.viewTextBacking = QColor(0, 0, 0, 72);
    tokens.displayFont = displayFontFamily();
    tokens.displayFontPixelSize = 16;
    tokens.viewTextureScale = 4;
    tokens.catPixelSize = 6;
    if (isDeepslate(m_variant)) {
        tokens.slotFill = QColor(0x2f, 0x2f, 0x35);
        tokens.slotShadow = QColor(0x12, 0x12, 0x15);
        tokens.slotHighlight = QColor(0x5c, 0x5c, 0x66);
        tokens.viewTexture = QStringLiteral(":/blocks/textures/deepslate.png");
        tokens.viewTextureTint = QColor(0, 0, 0, 120);
    } else {
        tokens.slotFill = QColor(0x8b, 0x8b, 0x8b);
        tokens.slotShadow = QColor(0x37, 0x37, 0x37);
        tokens.slotHighlight = Qt::white;
        tokens.viewTexture = QStringLiteral(":/blocks/textures/dirt.png");
        tokens.viewTextureTint = QColor(0, 0, 0, 150);
    }
    return tokens;
}

QList<std::pair<QByteArray, QFont>> BlocksTheme::widgetFonts()
{
    const QString family = displayFontFamily();
    if (family.isEmpty()) {
        return {};
    }
    QFont buttons(family);
    buttons.setPixelSize(14);
    buttons.setWeight(QFont::Medium);
    return { { "QPushButton", buttons }, { "QToolButton", buttons }, { "QTabBar", buttons } };
}

BlocksStyle::Colors BlocksTheme::styleColors() const
{
    BlocksStyle::Colors colors;
    colors.outline = Qt::black;
    colors.buttonOutlineActive = Qt::white;
    colors.primary = QColor(0x4f, 0x8f, 0x29);
    colors.primaryLight = QColor(0x86, 0xc4, 0x52);
    colors.primaryDark = QColor(0x2b, 0x55, 0x16);
    colors.field = Qt::black;
    colors.fieldBorderFocus = Qt::white;
    colors.xp = QColor(0x80, 0xd0, 0x20);
    colors.xpDark = QColor(0x4f, 0x8f, 0x14);
    colors.xpTrack = QColor(0x1a, 0x1a, 0x1a);
    colors.tooltip = QColor(0x10, 0x00, 0x10);
    colors.tooltipBorderTop = QColor(0x50, 0x00, 0xff);
    colors.tooltipBorderBottom = QColor(0x28, 0x00, 0x7f);
    colors.textShadow = QColor(0x3f, 0x3f, 0x3f);
    if (isDeepslate(m_variant)) {
        colors.button = QColor(0x55, 0x55, 0x5c);
        colors.buttonLight = QColor(0x7e, 0x7e, 0x88);
        colors.buttonDark = QColor(0x33, 0x33, 0x3a);
        colors.buttonDisabled = QColor(0x2a, 0x2a, 0x2f);
        colors.panel = QColor(0x2c, 0x2c, 0x31);
        colors.panelLight = QColor(0x4c, 0x4c, 0x55);
        colors.panelDark = QColor(0x14, 0x14, 0x17);
        colors.fieldBorder = QColor(0x6b, 0x6b, 0x75);
        colors.slot = QColor(0x1d, 0x1d, 0x21);
        colors.slotLight = QColor(0x4a, 0x4a, 0x52);
        colors.slotDark = QColor(0x0c, 0x0c, 0x0e);
    } else {
        colors.button = QColor(0x6f, 0x6f, 0x6f);
        colors.buttonLight = QColor(0xa5, 0xa5, 0xa5);
        colors.buttonDark = QColor(0x4a, 0x4a, 0x4a);
        colors.buttonDisabled = QColor(0x3c, 0x3c, 0x3c);
        colors.panel = QColor(0xc6, 0xc6, 0xc6);
        colors.panelLight = Qt::white;
        colors.panelDark = QColor(0x55, 0x55, 0x55);
        colors.fieldBorder = QColor(0xa0, 0xa0, 0xa0);
        colors.slot = QColor(0x8b, 0x8b, 0x8b);
        colors.slotLight = Qt::white;
        colors.slotDark = QColor(0x37, 0x37, 0x37);
    }
    return colors;
}
