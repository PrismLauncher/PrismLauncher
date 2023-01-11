// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls

Controls.Control {

// Palette setting for the Fusion theme, following the colors in the Qt Widgets world.
// https://doc.qt.io/qt-5/qml-palette.html

    SystemPalette { id: active_palette;   colorGroup: SystemPalette.Active   }
    SystemPalette { id: inactive_palette; colorGroup: SystemPalette.Inactive }
    SystemPalette { id: disabled_palette; colorGroup: SystemPalette.Disabled }

    palette {
        alternateBase: active_palette.alternateBase
        base: active_palette.base
        brightText: active_palette.brightText
        button: active_palette.button
        buttonText: active_palette.buttonText
        dark: active_palette.dark
        highlight: active_palette.highlight
        highlightedText: active_palette.highlightedText
        light: active_palette.light
        link: active_palette.link
        linkVisited: active_palette.linkVisited
        mid: active_palette.mid
        midlight: active_palette.midlight
        shadow: active_palette.shadow
        text: active_palette.text
        toolTipBase: active_palette.toolTipBase
        toolTipText: active_palette.toolTipText
        window: active_palette.window
        windowText: active_palette.windowText
    }

/* This works only on Qt6, but is by far the best solution :c
 * Leaving it here for future reference.

    palette.active {
        alternateBase: active_palette.alternativeBase
        base: active_palette.base
        brightText: active_palette.brightText
        button: active_palette.button
        buttonText: active_palette.buttonText
        dark: active_palette.dark
        highlight: active_palette.highlight
        highlightedText: active_palette.highlightedText
        light: active_palette.light
        link: active_palette.link
        linkVisited: active_palette.linkVisited
        mid: active_palette.mid
        midlight: active_palette.midlight
        shadow: active_palette.shadow
        text: active_palette.text
        toolTipBase: active_palette.toolTipBase
        toolTipText: active_palette.toolTipText
        window: active_palette.window
        windowText: active_palette.windowText
    }

    palette.inactive {
        alternateBase: inactive_palette.alternateBase
        base: inactive_palette.base
        brightText: inactive_palette.brightText
        button: inactive_palette.button
        buttonText: inactive_palette.buttonText
        dark: inactive_palette.dark
        highlight: inactive_palette.highlight
        highlightedText: inactive_palette.highlightedText
        light: inactive_palette.light
        link: inactive_palette.link
        linkVisited: inactive_palette.linkVisited
        mid: inactive_palette.mid
        midlight: inactive_palette.midlight
        shadow: inactive_palette.shadow
        text: inactive_palette.text
        toolTipBase: inactive_palette.toolTipBase
        toolTipText: inactive_palette.toolTipText
        window: inactive_palette.window
        windowText: inactive_palette.windowText
    }

    palette.disabled {
        alternateBase: disabled_palette.alternateBase
        base: disabled_palette.base
        brightText: disabled_palette.brightText
        button: disabled_palette.button
        buttonText: disabled_palette.buttonText
        dark: disabled_palette.dark
        highlight: disabled_palette.highlight
        highlightedText: disabled_palette.highlightedText
        light: disabled_palette.light
        link: disabled_palette.link
        linkVisited: disabled_palette.linkVisited
        mid: disabled_palette.mid
        midlight: disabled_palette.midlight
        shadow: disabled_palette.shadow
        text: disabled_palette.text
        toolTipBase: disabled_palette.toolTipBase
        toolTipText: disabled_palette.toolTipText
        window: disabled_palette.window
        windowText: disabled_palette.windowText
    }
*/
}
