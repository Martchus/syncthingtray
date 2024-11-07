import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

ExpandableDelegate {
    id: mainDelegateModel
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
        actions: [
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                enabled: !modelData.isThisDevice
                icon.source: App.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: (source) => App.connection[modelData.paused ? "resumeDevice" : "pauseDevice"]([modelData.devId])
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit")
                icon.source: App.faUrlBase + "pencil"
                onTriggered: (source) => mainView.stackView.push("DevConfigPage.qml", {devName: modelData.name, devId: modelData.devId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: App.faUrlBase + "cogs"
                onTriggered: (source) => mainView.stackView.push("AdvancedDevConfigPage.qml", {devName: modelData.name, devId: modelData.devId, stackView: mainView.stackView}, StackView.PushTransition)
            }
        ]
    }
}
