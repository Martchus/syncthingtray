import QtQuick
import QtQuick.Controls.Material

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
            wrapMode: TextEdit.WrapAnywhere
        }
    }
    property list<Action> actions: [
        Action {
            text: qsTr("Copy")
            icon.source: QuickUI.faUrlBase + "files-o"
            onTriggered: SyncthingModels.copyText(textArea.text)
        },
        Action {
            text: qsTr("Clear")
            icon.source: QuickUI.faUrlBase + "undo"
            onTriggered: {
                App.clearLog();
                textArea.clear();
            }
        }
    ]
}
