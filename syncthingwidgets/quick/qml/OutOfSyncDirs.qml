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
            icon.source: QuickUI.faUrlBase + "refresh"
            onTriggered: page.loadDirs()
        }
    ]
    contentItem: CustomListView {
        id: listView
        delegate: ItemDelegate {
            width: listView.width - (listView.ScrollBar?.vertical ? listView.ScrollBar.vertical.width : 0)
            text: qsTr("%1: %2 items needed, ~ %3").arg(modelData.dirName).arg(modelData.items).arg(SyncthingModels.formatDataSize(modelData.bytes))
            icon.source: QuickUI.faUrlBase + "folder-o"
            onClicked: page.stackView.push("NeededPage.qml", {devLabel: page.devLabel, devId: page.devId, dirLabel: modelData.dirName, dirId: modelData.dirId}, StackView.PushTransition)
            required property var modelData
        }
    }
    required property string devId
    required property string devLabel
    required property int devIndex
    required property var devFilterModel
    property StackView stackView: StackView {}
    required property list<Action> actions
    function loadDirs() {
        listView.model = SyncthingModels.computeDirsNeedingItems(devFilterModel.index(page.devIndex, 0));
    }
}
