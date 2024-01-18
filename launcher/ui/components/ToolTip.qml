// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls

/** Simple ToolTip wrapper that falls back to window colors when there's no toolTip colors in the palette.
 *  It also sets common delay and timeout values by default.
 */
Controls.ToolTip {
    id: root

    palette.toolTipText: active_palette.toolTipText ? active_palette.toolTipText
                                                    : active_palette.windowText
    palette.toolTipBase: active_palette.toolTipBase ? active_palette.toolTipBase
                                                    : active_palette.window

    delay: 500
    timeout: -1

    // NOTE: This is done so the ToolTip gets the contentWidth right and doesn't become too big.
    contentItem: Text {
        text: root.text
        color: palette.toolTipText
    }

    SystemPalette { id: active_palette; colorGroup: SystemPalette.Active }
}
