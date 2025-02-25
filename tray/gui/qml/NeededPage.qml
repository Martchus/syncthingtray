import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: neededPage
    title: devLabel.length > 0
           ? qsTr("Out of Sync - %1 on %2").arg(dirLabel).arg(devLabel)
           : qsTr("Out of Sync - %1").arg(dirLabel)
    Component.onCompleted: neededPage.loadItems()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: App.faUrlBase + "refresh"
            onTriggered: neededPage.refresh()
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
            width: listView.width
            contentItem: RowLayout {
                GridLayout {
                    Layout.fillWidth: true
                    columns: listView.width < 250 ? 1 : 2
                    Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        text: modelData.name ?? "?"
                        wrapMode: Text.WrapAnywhere
                        font.weight: Font.Light
                    }
                    Label {
                        Layout.alignment: Qt.AlignTop
                        text: App.formatDataSize(modelData.size ?? 0)
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                }
                IconOnlyButton {
                    text: qsTr("Move to top of queue")
                    icon.source: App.faUrlBase + "angle-double-up"
                    onClicked: App.requestFromSyncthing("POST", "db/prio", {folder: neededPage.dirId, file: modelData.name}, (res, error) => {})
                }
            }
            required property var modelData
        }
        section.property: "state"
        section.delegate: Pane {
            id: sectionDelegate
            width: listView.width
            contentItem: RowLayout {
                Label {
                    Layout.fillWidth: true
                    text: sectionDelegate.section
                    elide: Text.ElideRight
                    font.weight: Font.Medium
                }
            }
            required property string section
        }
        onAtYEndChanged: atYEnd && neededPage.loadItems()
        footer: LoadingPane {
            visible: neededPage.isRequestOngoing
            width: listView.width
        }
    }
    property string dirId
    property string dirLabel: dirId
    property string devId
    property string devLabel: devId
    property string route: devId.length > 0 ? "db/remoteneed" : "db/need"
    property bool isRequestOngoing: false
    property int page: 1
    property int perPage: 25
    property var params: devId.length > 0 ? ({folder: dirId, device: devId}) : ({folder: dirId})
    readonly property var stateLabels: ({
        progress: qsTr("In progress"),
        queued: qsTr("Queued"),
        rest: qsTr("Out of Sync"),
        files: qsTr("Out of Sync"),
    })
    required property list<Action> actions
    function loadItems() {
        if (isRequestOngoing) {
            return;
        }
        const paramsWithPaging = params;
        paramsWithPaging.page = page;
        paramsWithPaging.perpage = perPage;
        isRequestOngoing = true;
        App.requestFromSyncthing("GET", route, paramsWithPaging, (res, error) => {
            isRequestOngoing = false;
            if (page === 1) {
                listModel.clear();
            }
            page += 1;
            addItems(res, "progress");
            addItems(res, "queued");
            addItems(res, "rest");
            addItems(res, "files");
        });
    }
    function refresh() {
        page = 1;
        loadItems();
    }
    function addItems(data, state) {
        const stateData = data[state];
        if (!Array.isArray(stateData)) {
            return;
        }
        stateData.forEach((item) => {
            item.state = neededPage.stateLabels[state] ?? state;
            listModel.append(item);
        });
    }
}
