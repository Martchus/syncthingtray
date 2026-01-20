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
            mainModel: SyncthingModels.sortFilterDevModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add device")
                icon.source: QuickUI.faUrlBase + "plus"
                onTriggered: stackView.add()
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all")
                icon.source: QuickUI.faUrlBase + "pause"
                onTriggered: SyncthingData.connection.pauseAllDevs()
            },
            Action {
                text: qsTr("Resume all")
                icon.source: QuickUI.faUrlBase + "play"
                onTriggered: SyncthingData.connection.resumeAllDevs()
            }
        ]
        property alias model: devsListView.mainModel
    }
    function add(deviceId = "", deviceName = "") {
        stackView.push("DevConfigPage.qml", {devId: deviceId, devName: deviceName, existing: false, stackView: stackView}, StackView.PushTransition);
    }
}
