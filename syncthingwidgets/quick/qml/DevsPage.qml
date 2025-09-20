import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

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
                onTriggered: stackView.add()
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all")
                icon.source: App.faUrlBase + "pause"
                onTriggered: App.connection.pauseAllDevs()
            },
            Action {
                text: qsTr("Resume all")
                icon.source: App.faUrlBase + "play"
                onTriggered: App.connection.resumeAllDevs()
            }
        ]
        property alias model: devsListView.mainModel
    }
    function add(deviceId = "", deviceName = "") {
        stackView.push("DevConfigPage.qml", {devId: deviceId, devName: deviceName, existing: false, stackView: stackView}, StackView.PushTransition);
    }
}
