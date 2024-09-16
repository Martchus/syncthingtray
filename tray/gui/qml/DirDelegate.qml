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
                icon.source: app.faUrlBase + "refresh"
                onTriggered: (source) => app.connection.rescan(modelData.dirId)
            },
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                icon.source: app.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: (source) => app.connection[modelData.paused ? "resumeDirectories" : "pauseDirectories"]([modelData.dirId])
            },
            Action {
                text: qsTr("Open in file browser")
                icon.source: app.faUrlBase + "folder"
                onTriggered: (source) => app.openPath(modelData.path)
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Show errors")
                enabled: modelData.pullErrorCount > 0
                icon.source: app.faUrlBase + "exclamation-triangle"
                onTriggered: (source) => mainView.stackView.push("DirErrorsPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Edit ignore patterns")
                icon.source: app.faUrlBase + "filter"
                onTriggered: (source) => mainView.stackView.push("IgnorePatternPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Browse remote files")
                icon.source: app.faUrlBase + "folder-open-o"
                enabled: !modelData.paused
                onTriggered: (source) => mainView.stackView.push("FilesPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                icon.source: app.faUrlBase + "cogs"
                onTriggered: (source) => mainView.stackView.push("AdvancedDirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId, stackView: mainView.stackView}, StackView.PushTransition)
            }
        ]
    }
}
