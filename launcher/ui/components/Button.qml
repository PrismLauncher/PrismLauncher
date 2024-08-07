// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls

/** Simple Button wrapper that falls back to window colors when there's no button colors in the palette.
 *  It also takes care of changing the palette colors correctly on Qt5.
 */
Controls.Button {
    id: root

    function getTextColor() {
        if (enabled)
            return active_palette.buttonText ? active_palette.buttonText
                                             : active_palette.windowText
        else
            return disabled_palette.buttonText ? disabled_palette.buttonText
                                               : disabled_palette.windowText
    }

    function getBackgroundColor() {
        if (enabled)
            return active_palette.button ? active_palette.button
                                         : active_palette.window
        else
            return disabled_palette.button ? disabled_palette.button
                                           : disabled_palette.window
    }

    palette.buttonText: getTextColor()
    palette.button: getBackgroundColor()

    SystemPalette { id: active_palette;   colorGroup: SystemPalette.Active   }
    SystemPalette { id: disabled_palette; colorGroup: SystemPalette.Disabled }
}
