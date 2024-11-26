import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        id: page
        title: qsTr("Devices")
        Layout.fillWidth: true
        Layout.fillHeight: true
        DevListView {
            id: devsListView
            mainModel: App.sortFilterDevModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add device")
                icon.source: App.faUrlBase + "plus"
                onTriggered: (source) => page.add()
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all devices")
                icon.source: App.faUrlBase + "pause"
                onTriggered: (source) => App.connection.pauseAllDevs()
            },
            Action {
                text: qsTr("Resume all devices")
                icon.source: App.faUrlBase + "play"
                onTriggered: (source) => App.connection.resumeAllDevs()
            }
        ]
        property alias model: devsListView.mainModel
        function add() {
            stackView.push("DevConfigPage.qml", {devName: qsTr("New device"), devId: "", stackView: stackView}, StackView.PushTransition);
        }
    }
}
