import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
                onTriggered: (source) => App.connection.rescan(modelData.dirId)
            },
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                icon.source: App.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: (source) => App.connection[modelData.paused ? "resumeDirectories" : "pauseDirectories"]([modelData.dirId])
            },
            Action {
                text: qsTr("Open in file browser")
                icon.source: App.faUrlBase + "folder"
                onTriggered: (source) => App.openPath(modelData.path)
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit")
                icon.source: App.faUrlBase + "pencil"
                onTriggered: (source) => mainView.stackView.push("DirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Show errors")
                enabled: modelData.pullErrorCount > 0
                icon.source: App.faUrlBase + "exclamation-triangle"
                onTriggered: (source) => mainView.stackView.push("DirErrorsPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Edit ignore patterns")
                icon.source: App.faUrlBase + "filter"
                onTriggered: (source) => mainView.stackView.push("IgnorePatternPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Browse remote files")
                icon.source: App.faUrlBase + "folder-open-o"
                enabled: !modelData.paused
                onTriggered: (source) => mainView.stackView.push("FilesPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: App.faUrlBase + "cogs"
                onTriggered: (source) => mainView.stackView.push("AdvancedDirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            }
        ]
    }
}
