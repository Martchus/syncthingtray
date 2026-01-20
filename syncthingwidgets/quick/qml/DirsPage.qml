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
            mainModel: SyncthingModels.sortFilterDirModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add folder")
                icon.source: QuickUI.faUrlBase + "plus"
                onTriggered: stackView.add()
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all")
                icon.source: QuickUI.faUrlBase + "pause"
                onTriggered: SyncthingData.connection.pauseAllDirs()
            },
            Action {
                text: qsTr("Resume all")
                icon.source: QuickUI.faUrlBase + "play"
                onTriggered: SyncthingData.connection.resumeAllDirs()
            },
            Action {
                text: qsTr("Rescan all")
                icon.source: QuickUI.faUrlBase + "refresh"
                onTriggered: SyncthingData.connection.rescanAllDirs()
            }
        ]
        property alias model: dirsListView.mainModel
    }
    function add(dirId = "", dirName = "", shareWithDeviceIds = [], existing = false) {
        stackView.push("DirConfigPage.qml", {dirId: dirId, dirName: dirName, shareWithDeviceIds: shareWithDeviceIds ?? [], existing: existing, stackView: stackView}, StackView.PushTransition);
    }
}
