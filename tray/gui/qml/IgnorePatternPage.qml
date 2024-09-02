import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Ignore patterns of \"%1\"").arg(dirName)
    Component.onCompleted: app.loadIgnorePatterns(dirId, textArea)
    actions: [
        Action {
            text: qsTr("Save")
            icon.source: app.faUrlBase + "floppy-o"
            onTriggered: (source) => app.saveIgnorePatterns(dirId, textArea)
        }
    ]
    ScrollView {
        anchors.fill: parent
        TextArea {
            id: textArea
            anchors.fill: parent
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
