// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

import QtQml.Models 2.12

import QtQuick 2.12
import QtQuick.Controls 2.12

/** This code was heavily inspired by KDAB's proposed solution to a similar problem.
 *  See https://www.kdab.com/handling-a-lot-of-text-in-qml/ for more details. :)
 */

Rectangle {
    id: root

    color: palette.base

    property alias model: view.model
    property alias wrapMode: view.wrapMode

    // FIXME: Create a QML type to handle clipboard and use that instead
    signal requestedCopyToClipboard(string text)

    // TODO: Also highlight the specific text that matches the sequence
    function goToLine(line_number, highlight_entry) {
        if (line_number <= 0 || line_number > model.rowCount()) {
            view.currentIndex = -1;
            return;
        }

        if (highlight_entry) {
            view.highlightFollowsCurrentItem = true;
            view.currentIndex = line_number - 1;
        } else {
            view.currentIndex = -1;
        }

        view.positionViewAtIndex(line_number - 1, ListView.Beginning);
    }

    function goToTop() { view.positionViewAtBeginning(); }
    function goToBottom() { view.positionViewAtEnd(); }

    function getSelection() {
        var text = "";

        const indexes = Array.from(selectionModel.selectedIndexes);
        indexes.sort(function (a, b) { return a.row - b.row; });

        for (const index of indexes) {
            const item = view.itemAtIndex(index.row);

            // FIXME: This "approximates" the user selection on entries outside the current view. It should not :p
            if (!item)
                text += model.data(index);
            else
                text += item.selectedText;
        }

        return text;
    }

    ListView {
        id: view

        property int wrapMode: TextEdit.NoWrap
        property int fullWidth: 0

        function indexAtRelative(x, y) { return indexAt(x + contentX, y + contentY); }
        function updateContentWidth() {
            if (wrapMode === TextEdit.NoWrap)
                contentWidth = Qt.binding(() => { return view.fullWidth; });
            else
                contentWidth = Qt.binding(() => { return view.width; });
        }

        anchors.fill: parent
        clip: true

        smooth: false

        boundsBehavior: Flickable.StopAtBounds

        highlightMoveDuration: 1
        highlightMoveVelocity: -1

        flickDeceleration: 5000
        flickableDirection: Flickable.HorizontalAndVerticalFlick

        delegate: textDelegate

        highlight: Rectangle {
            color: "#1aff0000"
            border { color: "red" }
        }

        footerPositioning: ListView.PullBackFooter
        footer: Rectangle {
            width: parent.width
            height: 24
            color: "transparent"
        }

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar

            hoverEnabled: true
            policy: ScrollBar.AsNeeded
            minimumSize: 0.1

            width: 12
        }

        ScrollBar.horizontal: ScrollBar {
            id: horizontalScrollBar

            hoverEnabled: true
            policy: ScrollBar.AsNeeded
            minimumSize: 0.1

            height: 12
        }

        onModelChanged: {
            // Have no highlighting on page opening
            currentIndex = -1;

            selectionModel.model = view.model;
        }

        // FIXME: Only update this when the instance is running
        onCountChanged: {
            updatePositionTimer.start();
        }

        onWrapModeChanged: updateContentWidth()

        Component.onCompleted: updateContentWidth()

        Component {
            id: textDelegate

            TextArea {
                required property int index
                required property string line
                required property font text_font
                required property color foreground_color
                required property color background_color

                function updateSelection() {
                    const s_index = selectionArea.start_index;
                    const e_index = selectionArea.end_index;

                    if (!selectionModel.model)
                        return;

                    // If the item isn't in the selection at all
                    if (index < s_index || index > e_index) {
                        deselect();

                        // Removes this item from the current selections
                        const model_index = selectionModel.model.index(index, 0);
                        if (selectionModel.selectedIndexes.includes(model_index))
                            selectionModel.select(model_index, ItemSelectionModel.Deselect);
                        return;

                    // If the item is inside the selection, and isn't any of the edges of it
                    } else if (index > s_index && index < e_index) {
                        selectAll();

                    // If the item is the beginning and ending of the selection
                    } else if (index === s_index && index === e_index) {
                        select(selectionArea.start_position, selectionArea.end_position);

                    // If the selection starts at this item
                    } else if (index === s_index) {
                        select(selectionArea.start_position, length);

                    // If the selection ends at this item
                    } else if (index === e_index) {
                        select(0, selectionArea.end_position);
                    }

                    // Adds this item to the current selections
                    selectionModel.select(view.model.index(index, 0), ItemSelectionModel.Select);
                }

                padding: 0
                width: view.contentWidth
                renderType: Text.NativeRendering

                text: line
                textFormat: TextEdit.PlainText

                font: text_font
                color: foreground_color
                background: Rectangle {
                    // NOTE: The 8 is completely arbitrary and its only purpose is to make the rectangle look a bit nicer :)
                    width: parent.contentWidth + 8
                    color: background_color
                }

                readOnly: true
                selectByMouse: false

                wrapMode: view.wrapMode
                onWrapModeChanged: updateSelection()

                Component.onCompleted: {
                    updateSelection();

                    const new_content_width = contentWidth + verticalScrollBar.width;
                    view.fullWidth = Math.max(view.fullWidth, new_content_width);
                }

                Connections {
                    target: selectionArea
                    function onSelectionChanged() {
                        updateSelection();
                    }
                }
            }
        }

        // Based on https://forum.qt.io/topic/65028/qml-listview-scroll-to-last-added-item
        Timer {
            id: updatePositionTimer

            interval: 75
            repeat: false
            onTriggered: view.positionViewAtEnd()
        }

        ItemSelectionModel { id: selectionModel }
    }

    // TODO: Hook up onPressAndHold() for touch screens
    MouseArea {
        id: selectionArea

        anchors { left: parent.left; top: parent.top; }
        width: parent.width - verticalScrollBar.width
        height: parent.height - horizontalScrollBar.height

        hoverEnabled: true
        cursorShape: verticalScrollBar.hovered || horizontalScrollBar.hovered ? Qt.PointCursor : Qt.IBeamCursor

        acceptedButtons: Qt.LeftButton | Qt.RightButton

        // This is to prevent z-fighting with other elements in this hierarchy
        z: 9

        property int beginning_index: -1
        property int beginning_position: -1

        property int start_index: -1
        property int end_index: -1
        property int start_position: -1
        property int end_position: -1

        signal selectionChanged

        onPressed: (event) => {
            if (event.button !== Qt.LeftButton)
                return;

            const [index, pos] = indexAndPos(mouseX, mouseY);
            if (!index)
                return;

            selectionModel.clear();

            [beginning_index, beginning_position] = [index, pos];

            [start_index, start_position] = [beginning_index, beginning_position];
            [end_index, end_position] = [beginning_index, beginning_position];

            selectionChanged();

            view.highlightFollowsCurrentItem = false;
            view.forceActiveFocus();
        }

        onClicked: (event) => {
            if (event.button !== Qt.RightButton)
                return;

            rightClickMenu.popup();
        }

        onPositionChanged: {
            if (!pressed)
                return;

            const [index, position] = indexAndPos(mouseX, mouseY);
            if (!index)
                return;

            if (index < beginning_index || (index === beginning_index && position < beginning_position)) {
                [start_index, start_position] = [index, position];
                [end_index, end_position] = [beginning_index, beginning_position];
            } else {
                [start_index, start_position] = [beginning_index, beginning_position];
                [end_index, end_position] = [index, position];
            }

            selectionChanged();
        }

        function indexAndPos(x, y) {
            const index = view.indexAtRelative(x, y);
            if (index === -1)
                return [];
            const item = view.itemAtIndex(index);
            const relItemX = item.x - view.contentX;
            const relItemY = item.y - view.contentY;
            const pos = item.positionAt(x - relItemX, y - relItemY);

            return [index, pos];
        }
    }

    Menu {
        id: rightClickMenu

        Action {
            id: copy_action

            text: qsTr("&Copy selection")
            icon.name: "edit-copy"
            shortcut: StandardKey.Copy

            onTriggered: requestedCopyToClipboard(getSelection())
        }
    }
}
