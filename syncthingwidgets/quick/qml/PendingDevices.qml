import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: page
    title: qsTr("Pending devices")
    Component.onCompleted: page.load()
    actions: [
        Action {
            text: qsTr("Refresh")
            icon.source: App.faUrlBase + "refresh"
            onTriggered: page.load()
        }
    ]
    contentItem: CustomListView {
        id: listView
        delegate: ItemDelegate {
            id: itemDelegate
            width: listView.width
            contentItem: ColumnLayout {
                width: listView.width
                Label {
                    Layout.fillWidth: true
                    text: App.deviceDisplayName(modelData.devId)
                    elide: Text.ElideRight
                    font.weight: Font.Medium
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Name: ") + (modelData.info.Name ?? modelData.info.name)
                    elide: Text.ElideRight
                    font.weight: Font.Light
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Address: ") + (modelData.info.Address ?? modelData.info.address)
                    elide: Text.ElideRight
                    font.weight: Font.Light
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("Ignore")
                        flat: true
                        onClicked: page.ignoreDev(modelData.devId)
                    }
                    Button {
                        text: qsTr("Add device")
                        flat: true
                        onClicked: pages.addDevice(modelData.devId, modelData.info.Name ?? modelData.info.name)
                    }
                }
            }
            required property var modelData
        }
        model: ListModel {
            id: listModel
        }
    }

    required property var pages
    required property list<Action> actions

    function load() {
        App.requestFromSyncthing("GET", "cluster/pending/devices", {}, (res, error) => {
            listModel.clear();
            if (error.length > 0) {
                return;
            }
            Object.entries(res).forEach((pendingDev) => {
                listModel.append({devId: pendingDev[0], info: pendingDev[1]});
            });
        });
    }

    function ignoreDev(devId, info) {
        const cfg = App.connection.rawConfig;
        const ignoredDevs = cfg.remoteIgnoredDevices;
        if (!Array.isArray(ignoredDevs)) {
            return false;
        }
        ignoredDevs.push({deviceID: devId, name: info.Name ?? info.name ?? "", address: info.Address ?? info.address ?? "", time: new Date().toISOString()});
        App.postSyncthingConfig(cfg, (error) => {
            if (error.length === 0) {
                page.load();
            }
        });
        return true;
    }
}
