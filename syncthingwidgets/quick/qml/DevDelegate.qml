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
                icon.source: QuickUI.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: SyncthingData.connection[modelData.paused ? "resumeDevice" : "pauseDevice"]([modelData.devId])
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit")
                icon.source: QuickUI.faUrlBase + "pencil"
                onTriggered: QuickUI.editDev(modelData.devId, modelData.name, mainView.stackView)
            },
            Action {
                text: qsTr("Out of Sync items")
                icon.source: QuickUI.faUrlBase + "exchange"
                enabled: !modelData.paused && (modelData.neededItemsCount > 0)
                onTriggered: QuickUI.showOutOfSyncDirs(modelData.devId, modelData.name, modelData.index, mainDelegateModel.model, mainView.stackView)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: QuickUI.faUrlBase + "cogs"
                onTriggered: QuickUI.editDev(modelData.devId, modelData.name, mainView.stackView, true)
            }
        ]
    }
}
