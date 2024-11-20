import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        title: qsTr("Folders")
        Layout.fillWidth: true
        Layout.fillHeight: true
        DirListView {
            mainModel: App.sortFilterDirModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add folder")
                icon.source: App.faUrlBase + "plus"
                onTriggered: (source) => stackView.push("DirConfigPage.qml", {dirName: qsTr("New folder"), dirId: "", stackView: stackView}, StackView.PushTransition)
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all folders")
                icon.source: App.faUrlBase + "pause"
                onTriggered: (source) => App.connection.pauseAllDirs()
            },
            Action {
                text: qsTr("Resume all folders")
                icon.source: App.faUrlBase + "play"
                onTriggered: (source) => App.connection.resumeAllDirs()
            },
            Action {
                text: qsTr("Rescan all folders")
                icon.source: App.faUrlBase + "refresh"
                onTriggered: (source) => App.connection.rescanAllDirs()
            }
        ]
    }
}
