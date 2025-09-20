import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: page
    title: qsTr("Out of Sync folders on %1").arg(devLabel)
    Component.onCompleted: page.loadDirs()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: App.faUrlBase + "refresh"
            onTriggered: page.loadDirs()
        }
    ]
    contentItem: CustomListView {
        id: outOfSyncDirsListView
        delegate: ItemDelegate {
            width: outOfSyncDirsListView.width
            text: qsTr("%1: %2 items needed, ~ %3").arg(modelData.dirName).arg(modelData.items).arg(App.formatDataSize(modelData.bytes))
            icon.source: App.faUrlBase + "folder-o"
            onClicked: page.stackView.push("NeededPage.qml", {devLabel: page.devLabel, devId: page.devId, dirLabel: modelData.dirName, dirId: modelData.dirId}, StackView.PushTransition)
            required property var modelData
        }
    }
    required property string devId
    required property string devLabel
    required property int dirIndex
    required property var devFilterModel
    required property var stackView
    required property list<Action> actions
    function loadDirs() {
        outOfSyncDirsListView.model = App.computeDirsNeedingItems(devFilterModel.index(dirIndex, 0));
    }
}
