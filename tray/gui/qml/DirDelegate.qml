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
                text: qsTr("Edit ignore patterns")
                onTriggered: (source) => mainView.stackView.push("IgnorePatternPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Browse remote files")
                enabled: !modelData.paused
                onTriggered: (source) => mainView.stackView.push("FilesPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                onTriggered: (source) => mainView.stackView.push("AdvancedDirConfigPage.qml", {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            }
        ]
    }
}
