import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Page {
    title: qsTr("Errors of folder \"%1\"").arg(dirName)
    Component.onCompleted: app.loadDirErrors(dirId, listView)
    ScrollView {
        anchors.fill: parent
        ListView {
            id: listView
            width: parent.width
            enabled: false
            delegate: ItemDelegate {
                width: listView.width
                contentItem: ColumnLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: modelData.path
                        wrapMode: Text.WrapAnywhere
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.fillWidth: true
                        text: modelData.message
                        wrapMode: Text.WrapAnywhere
                        font.weight: Font.Light
                    }
                }
                required property var modelData
            }
        }
    }
    BusyIndicator {
        anchors.centerIn: parent
        running: !listView.enabled
    }

    required property string dirName
    required property string dirId
    property list<Action> actions
}
