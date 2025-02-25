import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ExpandableDelegate {
    id: mainDelegateModel
    delegate: ExpandableItemDelegate {
        id: devDelegate
        mainView: mainDelegateModel.mainView
        actions: [
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                enabled: !modelData.isThisDevice
                icon.source: App.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: App.connection[modelData.paused ? "resumeDevice" : "pauseDevice"]([modelData.devId])
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit")
                icon.source: App.faUrlBase + "pencil"
                onTriggered: mainView.stackView.push("DevConfigPage.qml", {devName: modelData.name, devId: modelData.devId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Out of Sync items")
                icon.source: App.faUrlBase + "exchange"
                enabled: !modelData.paused && (modelData.neededItemsCount > 0)
                onTriggered: mainView.stackView.push("OutOfSyncDirs.qml", {devLabel: modelData.name, devId: modelData.devId, dirIndex: modelData.index, devFilterModel: mainDelegateModel.model, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: App.faUrlBase + "cogs"
                onTriggered: mainView.stackView.push("AdvancedDevConfigPage.qml", {devName: modelData.name, devId: modelData.devId, stackView: mainView.stackView}, StackView.PushTransition)
            }
        ]
    }
}
