import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

Page {
    id: neededPage
    title: devLabel.length > 0
           ? qsTr("Out of sync - %1 on %2").arg(dirLabel).arg(devLabel)
           : qsTr("Out of sync - %1").arg(dirLabel)
    Component.onCompleted: neededPage.loadItems()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: App.faUrlBase + "refresh"
            onTriggered: (source) => neededPage.loadItems()
        }
    ]
    CustomListView {
        id: listView
        anchors.fill: parent
        model: ListModel {
            id: listModel
            dynamicRoles: true
        }
        delegate: ItemDelegate {
            text: modelData.name ?? "?"
            onClicked: App.requestFromSyncthing("POST", "db/prio", {folder: neededPage.dirId, file: modelData.name}, (res, error) => {})
            required property var modelData
        }
    }
    property string dirId
    property string dirLabel: dirId
    property string devId
    property string devLabel: devId
    property string route: devId.length > 0 ? "db/remoteneed" : "db/need"
    property var params: devId.length > 0 ? ({folder: dirId, device: devId}) : ({folder: dirId})
    required property list<Action> actions
    function loadItems() {
        App.requestFromSyncthing("GET", route, params, (res, error) => {
            addItems(res, "progress");
            addItems(res, "queued");
            addItems(res, "rest");
            addItems(res, "files");
        });
    }
    function addItems(data, state) {
        const stateData = data[state];
        if (!Array.isArray(stateData)) {
            return;
        }
        stateData.forEach((item) => {
            item.state = state;
            listModel.add(item);
        });
    }
}
