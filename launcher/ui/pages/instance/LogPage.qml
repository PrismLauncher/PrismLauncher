// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "../../components" as Components

// FIXME: Disable buttons when the action doesn't do anything
Components.ThemedControl {
    id: page

    property alias logModel: logView.model
    property alias wrapMode: logView.wrapMode
    property bool suspended: false
    property bool useRegexInSearch: false

    signal onWrapModeChanged(int new_state)
    signal onSuspendedChanged(bool new_state)
    signal onUseRegexChanged(bool new_state)

    signal onCopyPressed(string to_copy)
    signal onUploadPressed
    signal onClearPressed
    
    signal onSearchRequested(string regex, bool reverse)

    function gotSourceModel() { suspended_checkbox.enabled = true; }
    function lostSourceModel() { suspended_checkbox.enabled = false; }

    function goToLine(line_number, highlight_start, highlight_end) { logView.goToLine(line_number, highlight_start, highlight_end); }
    function search(reverse) { page.onSearchRequested(search_input_field.text, reverse); }

    SystemPalette { id: active_palette; colorGroup: SystemPalette.Active }

    Shortcut {
        sequences: [ StandardKey.Find ]
        onActivated: search_input_field.focus = true
    }

    Shortcut {
        sequences: [ StandardKey.FindNext ]
        onActivated: page.search(false)
    }

    Shortcut {
        sequences: [ StandardKey.FindPrevious ]
        onActivated: page.search(true)
    }

    Rectangle {
        anchors.fill: parent
        color: active_palette.window
        Layout.preferredWidth: 825

        ColumnLayout {
            anchors { fill: parent; rightMargin: 6 }

            RowLayout {
                id: top_row

                Layout.fillWidth: true
                implicitHeight: 48

                CheckBox {
                    id: suspended_checkbox
                    text: qsTr("Keep updating")

                    enabled: false

                    checked: !page.suspended
                    onToggled: page.onSuspendedChanged( !page.suspended )
                }

                CheckBox {
                    text: qsTr("Wrap lines")

                    checked: page.wrapMode === TextEdit.WordWrap
                    onToggled: page.onWrapModeChanged( page.wrapMode === TextEdit.WordWrap ? TextEdit.NoWrap : TextEdit.WordWrap )
                }

                Item {
                    id: top_row_separator
                    Layout.fillWidth: true
                }

                Components.Button {
                    text: qsTr("Copy")

                    onPressed: page.onCopyPressed("")

                    hoverEnabled: true
                    Components.ToolTip {
                        text: qsTr("Copies the entire logs to the clipboard.<br>To copy only a selection, use Ctrl+C instead.")
                        visible: parent.hovered
                    }
                }

                Components.Button {
                    text: qsTr("Upload")

                    onPressed: page.onUploadPressed()

                    hoverEnabled: true
                    Components.ToolTip {
                        text: qsTr("Uploads the log to the paste service configured in the launcher settings.")
                        visible: parent.hovered
                    }
                }

                Components.Button {
                    text: qsTr("Clear")

                    onPressed: page.onClearPressed()

                    hoverEnabled: true
                    Components.ToolTip {
                        text: qsTr("Clears the entire log.")
                        visible: parent.hovered
                    }
                }
            }

            Components.ListBasedTextArea {
                id: logView

                Layout.fillWidth: true
                Layout.fillHeight: true

                onRequestedCopyToClipboard: (text) => { page.onCopyPressed(text); }
            }

            RowLayout {
                id: bottomRow

                Layout.fillWidth: true
                implicitHeight: 28

                property bool isShiftPressed: false
                Keys.onPressed: (event) => { if (event.key === Qt.Key_Shift) isShiftPressed = true; }
                Keys.onReleased: (event) => { if (event.key === Qt.Key_Shift) isShiftPressed = false; }

                TextField {
                    id: search_input_field

                    Layout.fillWidth: true
                    implicitHeight: parent.implicitHeight

                    placeholderText: qsTr("Search...")
                    placeholderTextColor: active_palette.placeholderText || Qt.tint(active_palette.text, "#50404040")

                    onAccepted: page.search(bottomRow.isShiftPressed)
                }

                CheckBox {
                    id: regexCheckBox
                    text: qsTr("Treat as RegEx")

                    checked: page.useRegexInSearch
                    onToggled: page.onUseRegexChanged( !page.useRegexInSearch )

                    hoverEnabled: true
                    Components.ToolTip {
                        text: qsTr("If enabled, interpret the search input as a regular expression.<br>Otherwise, do a simple text search.")
                        visible: regexCheckBox.hovered
                    }
                }

                Components.Button {
                    text: qsTr("Search")

                    onClicked: search_input_field.accepted()
                }

                Rectangle {
                    implicitWidth: 1
                    implicitHeight: parent.height
                    color: "gray"
                }

                Components.Button {
                    // FIXME: This should be an icon-only button with a "go-bottom" icon. Need to make the icon though :<
                    text: qsTr("Bottom")

                    onClicked: logView.goToBottom()

                    hoverEnabled: true
                    Components.ToolTip {
                        text: qsTr("Scrolls the log to the bottom in one go.")
                        visible: parent.hovered
                    }
                }
            }
        }
    }
}
