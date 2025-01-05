import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    title: qsTr("Ignore patterns of \"%1\"").arg(dirName)
    Component.onCompleted: App.loadIgnorePatterns(dirId, textArea)
    actions: [
        Action {
            text: qsTr("Save")
            icon.source: App.faUrlBase + "floppy-o"
            onTriggered: (source) => App.saveIgnorePatterns(dirId, textArea)
        }
    ]
    ScrollView {
        anchors.fill: parent
        TextArea {
            id: textArea
            width: parent.width
            enabled: false
        }
    }
    BusyIndicator {
        anchors.centerIn: parent
        running: !textArea.enabled
    }

    required property string dirName
    required property string dirId
    required property list<Action> actions
}
