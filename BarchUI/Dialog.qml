import QtQuick

// MouseArea prevents the parent element from receiving touch events.
MouseArea {
    id: root

    property string text: "<Dialog Text>"

    signal okClicked

    anchors.fill: parent

    // An old school shadow.
    Rectangle {
        width:   0.8 * parent.width
        height:  0.8 * parent.height
        color:   'gray'
        opacity: 0.5
        anchors {
            centerIn: parent
            verticalCenterOffset:   0.03 * parent.height
            horizontalCenterOffset: 0.03 * parent.width
        }
    }

    Rectangle {
        width:  0.8 * parent.width
        height: 0.8 * parent.height

        border.color: 'black'
        border.width: 0.01 * height

        anchors.centerIn: parent

        Text {
            text: root.text
            anchors {
                centerIn: parent
                verticalCenterOffset: -0.1 * parent.height
            }
            width: 0.95 * parent.width
            wrapMode: Text.WordWrap
            font.pixelSize: 0.1 * Math.min(parent.width, parent.height)
            horizontalAlignment: Text.AlignHCenter
        }

        Button {
            id: ok
            width:  0.5 * parent.width
            height: 0.2 * parent.height
            text: "OK"
            onClicked: root.okClicked()
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: 0.1 * parent.height
            }
        }
    }
}
