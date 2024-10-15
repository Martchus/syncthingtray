import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Syncthing log")
    Component.onCompleted: app.showLog(textArea)
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
            text: qsTr("Clear")
            icon.source: app.faUrlBase + "undo"
            onTriggered: (source) => {
                app.clearLog();
                textArea.clear();
            }
        }
    ]
}
