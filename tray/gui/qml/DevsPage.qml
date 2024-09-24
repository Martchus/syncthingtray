import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        title: qsTr("Devices")
        Layout.fillWidth: true
        Layout.fillHeight: true
        DevListView {
            mainModel: app.devModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add device")
                icon.source: app.faUrlBase + "plus"
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all devices")
                icon.source: app.faUrlBase + "pause"
                onTriggered: (source) => app.connection.pauseAllDevs()
            },
            Action {
                text: qsTr("Resume all devices")
                icon.source: app.faUrlBase + "play"
                onTriggered: (source) => app.connection.resumeAllDevs()
            }
        ]
    }
}
