import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

Page {
    title: qsTr("Syncthing log")
    Component.onCompleted: App.showLog(textArea)
    ScrollView {
        anchors.fill: parent
        TextArea {
            id: textArea
            width: parent.width
            readOnly: true
        }
    }
    property list<Action> actions: [
        Action {
            text: qsTr("Copy")
            icon.source: App.faUrlBase + "files-o"
            onTriggered: App.copyText(textArea.text)
        },
        Action {
            text: qsTr("Clear")
            icon.source: App.faUrlBase + "undo"
            onTriggered: (source) => {
                App.clearLog();
                textArea.clear();
            }
        },
    ]
}
