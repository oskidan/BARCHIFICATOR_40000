import QtQuick

// Make it a child of a ListView or a GridView.

Rectangle {
    color: 'gray'
    width: 5.0;  radius: 4
    anchors {
        right: 	 parent.right
        margins: radius
    }

    height:  parent.height   / parent.contentHeight * parent.height
    y:       parent.contentY / parent.contentHeight * parent.height
    visible: parent.height   < parent.contentHeight
}
