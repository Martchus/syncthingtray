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
        title: qsTr("Folders")
        Layout.fillWidth: true
        Layout.fillHeight: true
        DirListView {
            id: dirsListView
            mainModel: App.sortFilterDirModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add folder")
                icon.source: App.faUrlBase + "plus"
                onTriggered: (source) => page.add()
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all")
                icon.source: App.faUrlBase + "pause"
                onTriggered: (source) => App.connection.pauseAllDirs()
            },
            Action {
                text: qsTr("Resume all")
                icon.source: App.faUrlBase + "play"
                onTriggered: (source) => App.connection.resumeAllDirs()
            },
            Action {
                text: qsTr("Rescan all")
                icon.source: App.faUrlBase + "refresh"
                onTriggered: (source) => App.connection.rescanAllDirs()
            }
        ]
        property alias model: dirsListView.mainModel
        function add(dirId = "", dirName = "", shareWithDeviceIds = []) {
            stackView.push("DirConfigPage.qml", {dirId: dirId, dirName: dirName, shareWithDeviceIds: shareWithDeviceIds ?? [], stackView: stackView}, StackView.PushTransition);
        }
    }
}
