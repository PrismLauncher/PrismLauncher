import QtQuick 2.0

GridView {
    id: grid
    model: instances
    anchors.fill: parent
    cellWidth: iconSize*2
    cellHeight: iconSize*2

    highlight: Rectangle {
        width: grid.cellWidth; height: grid.cellHeight
        color: "lightsteelblue"; radius: 5
        x: grid.currentItem.x
        y: grid.currentItem.y
        Behavior on x { SmoothedAnimation { duration: 150 } }
        Behavior on y { SmoothedAnimation { duration: 150 } }
    }

    interactive: true
    focus: true

    delegate: Item {
        required property int index
        required property string name
        required property string icon
        width: iconSize*2
        height: iconSize*2

        MouseArea {
            anchors.fill: parent
            onClicked: currentIndex = index;
        }

        Image {
            id: icon
            width: iconSize
            height: iconSize
            anchors.top: parent.top
            source: parent.icon
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            anchors.top: icon.bottom
            text: parent.name
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
