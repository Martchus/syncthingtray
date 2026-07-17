import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: neededPage
    title: neededPage.localChanged
           ? qsTr("Locally changed - %1").arg(dirLabel)
           : devLabel.length > 0
             ? qsTr("Out of Sync - %1 on %2").arg(dirLabel).arg(devLabel)
             : qsTr("Out of Sync - %1").arg(dirLabel)
    Component.onCompleted: neededPage.loadItems()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: QuickUI.faUrlBase + "refresh"
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
            width: listView.width - (listView.ScrollBar?.vertical ? listView.ScrollBar.vertical.width : 0)
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
                        text: SyncthingModels.formatDataSize(modelData.size ?? 0)
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                }
                IconOnlyButton {
                    text: qsTr("Move to top of queue")
                    icon.source: QuickUI.faUrlBase + "angle-double-up"
                    visible: !neededPage.localChanged && devId.length === 0
                    onClicked: SyncthingModels.requestFromSyncthing("POST", "db/prio", {folder: neededPage.dirId, file: modelData.name}, (res, error) => {})
                }
            }
            required property var modelData
        }
        section.property: "state"
        section.delegate: DynamicSectionHeader {
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
    property bool localChanged: false
    property bool isRequestOngoing: false
    property string route: localChanged ? "db/localchanged" : devId.length > 0 ? "db/remoteneed" : "db/need"
    property int page: 1
    property int perPage: 25
    property StackView stackView
    property var pageWindow
    readonly property var stateLabels: ({
        progress: qsTr("In progress"),
        queued: qsTr("Queued"),
        rest: qsTr("Out of Sync"),
        files: localChanged ? qsTr("Locally changed") : qsTr("Out of Sync"),
    })
    required property list<Action> actions
    function loadItems() {
        if (isRequestOngoing) {
            return;
        }
        const paramsWithPaging = devId.length > 0 ? ({folder: dirId, device: devId}) : ({folder: dirId});
        paramsWithPaging.page = page;
        paramsWithPaging.perpage = perPage;
        isRequestOngoing = true;
        SyncthingModels.requestFromSyncthing("GET", route, paramsWithPaging, (res, error) => {
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
