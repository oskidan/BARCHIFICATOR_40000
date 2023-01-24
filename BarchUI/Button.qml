import QtQuick

Rectangle {
    id: root

    property string text: '<Button Label>'

    signal clicked()

    width: 150; height: 50

    border.color: text? 'black' : 'transparent'
    border.width: 0.05 * height
    radius:       0.5  * height

    opacity: enabled && !mouseArea.pressed? 1.0 : 0.3
    clip: true

    Text {
        text: root.text
        font.pixelSize: 0.5 * Math.min(parent.width, parent.height)
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
