import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt.labs.qmlmodels

import Main

ExpandableDelegate {
    id: mainDelegateModel
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
        actions: [
            Action {
                text: qsTr("Rescan")
                enabled: !modelData.paused
                icon.source: QuickUI.faUrlBase + "refresh"
                onTriggered: SyncthingData.connection.rescan(modelData.dirId)
            },
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                icon.source: QuickUI.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: SyncthingData.connection[modelData.paused ? "resumeDirectories" : "pauseDirectories"]([modelData.dirId])
            },
            Action {
                text: qsTr("Open")
                icon.source: QuickUI.faUrlBase + "folder"
                onTriggered: SyncthingModels.openPath(modelData.path)
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit")
                icon.source: QuickUI.faUrlBase + "pencil"
                onTriggered: QuickUI.showDir(modelData.dirId, modelData.name) || mainView.stackView.push("DirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Out of Sync items")
                icon.source: QuickUI.faUrlBase + "exchange"
                enabled: !modelData.paused && (modelData.neededItemsCount > 0)
                onTriggered: mainView.stackView.push("NeededPage.qml", {dirLabel: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: modelData.overrideRevertActionLabel
                icon.source: QuickUI.faUrlBase + "undo"
                enabled: modelData.overrideRevertAction.length > 0
                onTriggered: mainView.confirmDirAction(modelData.dirId, modelData.name, modelData.overrideRevertAction)
            },
            Action {
                text: qsTr("Show errors")
                enabled: modelData.pullErrorCount > 0
                icon.source: QuickUI.faUrlBase + "exclamation-triangle"
                onTriggered: mainView.stackView.push("DirErrorsPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Ignore patterns")
                icon.source: QuickUI.faUrlBase + "filter"
                onTriggered: mainView.stackView.push("IgnorePatternPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Remote files")
                icon.source: QuickUI.faUrlBase + "folder-open-o"
                enabled: !modelData.paused
                onTriggered: mainView.stackView.push("FilesPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: QuickUI.faUrlBase + "cogs"
                onTriggered: mainView.stackView.push("AdvancedDirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Media rescan")
                icon.source: QuickUI.faUrlBase + "music"
                enabled: SyncthingModels.scanSupported
                onTriggered: SyncthingModels.scanPath(modelData.path)
            }
        ]
    }
}
