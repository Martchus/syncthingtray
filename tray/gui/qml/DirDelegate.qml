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
                icon.source: App.faUrlBase + "refresh"
                onTriggered: App.connection.rescan(modelData.dirId)
            },
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                icon.source: App.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: App.connection[modelData.paused ? "resumeDirectories" : "pauseDirectories"]([modelData.dirId])
            },
            Action {
                text: qsTr("Open")
                icon.source: App.faUrlBase + "folder"
                onTriggered: App.openPath(modelData.path)
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit")
                icon.source: App.faUrlBase + "pencil"
                onTriggered: mainView.stackView.push("DirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Out of Sync items")
                icon.source: App.faUrlBase + "exchange"
                enabled: !modelData.paused && (modelData.neededItemsCount > 0)
                onTriggered: mainView.stackView.push("NeededPage.qml", {dirLabel: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: modelData.overrideRevertActionLabel
                icon.source: App.faUrlBase + "undo"
                enabled: modelData.overrideRevertAction.length > 0
                onTriggered: mainView.confirmDirAction(modelData.dirId, modelData.name, modelData.overrideRevertAction)
            },
            Action {
                text: qsTr("Show errors")
                enabled: modelData.pullErrorCount > 0
                icon.source: App.faUrlBase + "exclamation-triangle"
                onTriggered: mainView.stackView.push("DirErrorsPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Ignore patterns")
                icon.source: App.faUrlBase + "filter"
                onTriggered: mainView.stackView.push("IgnorePatternPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Remote files")
                icon.source: App.faUrlBase + "folder-open-o"
                enabled: !modelData.paused
                onTriggered: mainView.stackView.push("FilesPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: App.faUrlBase + "cogs"
                onTriggered: mainView.stackView.push("AdvancedDirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Media rescan")
                icon.source: App.faUrlBase + "music"
                enabled: App.scanSupported
                onTriggered: App.scanPath(modelData.path)
            }
        ]
    }
}
