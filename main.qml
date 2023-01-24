import QtQuick

import BarchUI

Window {
    id: mainWindow

    width:   500
    height:  400
    visible: true

    title: qsTr("BARCHIFICATOR 40000")

    ListView {
        id: 		  fileView
        anchors.fill: parent

        // See: main.cpp
        model: files

        delegate: Row {
            id: fileDelegate

            padding: 30

            // Hide files that are not BMP, PNG, or BARCH ones.
            visible: /.*\.(bmp|png|barch)$/.test(name)

            // Shrink them down to 0 height. Otherwise we'll see blank space in the ListView.
            // This is the simplest (though less performant) way of doing this. Requires zero
            // change to the model.
            height: visible ? childrenRect.height + 20 : 0.0

            // Displays the file name. Clicking on it will create an encoding/decoding task depending on the file type.
            Button {
                id: fileButton
                text: name
                width: 0.7 * mainWindow.width
                onClicked: transcode()
            }

            // Displays the file size in kilobytes. I rotated it just to make the UI a bit fancy.
            Text{
                text: `${Math.round(size / 1024)} KB.`
                anchors.verticalCenter: fileButton.verticalCenter
                font.pixelSize: 0.3 * Math.min(fileButton.width, fileButton.height)
                rotation: -35
            }

            // Displays the progress.
            Text {
                visible: progress !== 0
                text: `${progress}%`
                anchors.verticalCenter: fileButton.verticalCenter
                font.pixelSize: 0.3 * Math.min(fileButton.width, fileButton.height)
                rotation: -35
            }

            property int initialX: x
            Component.onCompleted: {
                // Break initialX binding.
                initialX = initialX
            }

            // I wanted to make the UI a bit more responsive. This simple animation is meant to be a clue for the user
            // that something went wrong.
            SequentialAnimation {
                id: errorAnimation
                NumberAnimation { target: fileDelegate; property: "x"; to:  60;      duration: 66 }
                NumberAnimation { target: fileDelegate; property: "x"; to: -60;      duration: 66 }
                NumberAnimation { target: fileDelegate; property: "x"; to:  60;      duration: 66 }
                NumberAnimation { target: fileDelegate; property: "x"; to: -60;      duration: 66 }
                NumberAnimation { target: fileDelegate; property: "x"; to: initialX; duration: 66 }
                running: false
                property string message: "<Error Message>"
                onStopped: {
                    let dialogComponent = Qt.createComponent(
                            'qrc:/oleksii.skidan/imports/BarchUI/Dialog.qml')
                    let dialog = dialogComponent.createObject(mainWindow, {
                                                                  "parent": mainWindow,
                                                                  "text": `${message}`
                                                              })
                    dialog.okClicked.connect(() => dialog.destroy())
                }
            }

            // This is the animation that's played back on success.
            SequentialAnimation {
                id: successAnimation
                ColorAnimation { target: fileButton; property: "color"; from: "transparent"; to: "black"; duration: 66 }
                ColorAnimation { target: fileButton; property: "color"; from: "black"; to: "transparent"; duration: 66 }
                running: false
            }

            // Wire up the animations to the model.
            Connections {
                target: currentFile
                function onSuccess() {
                    successAnimation.running = true
                }
                function onError(message) {
                    errorAnimation.message = message
                    errorAnimation.start()
                }
            }
        }

        ScrollBar {}
    }
}
