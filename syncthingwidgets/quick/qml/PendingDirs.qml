import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: page
    title: qsTr("Pending folders")
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
                    text: App.dirDisplayName(modelData.dirId)
                    elide: Text.ElideRight
                    font.weight: Font.Medium
                    wrapMode: Text.WordWrap
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Offered by:")
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                    Repeater {
                        id: devRepeater
                        model: modelData.devs
                        ItemDelegate {
                            Layout.fillWidth: true
                            onClicked: {
                                devCheckBox.toggle();
                                devCheckBox.toggled();
                            }
                            contentItem: RowLayout {
                                CheckBox {
                                    id: devCheckBox
                                    checked: modelData.checked
                                    onCheckedChanged: devRepeater.model.setProperty(modelData.index, "checked", devCheckBox.checked)
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: App.deviceDisplayName(modelData.devId)
                                    elide: Text.ElideRight
                                    font.weight: Font.Light
                                    wrapMode: Text.WordWrap
                                }
                            }
                            required property var modelData
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("For selected devices:")
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: qsTr("Ignore")
                            flat: true
                            onClicked: page.ignoreDir(modelData.dirId, itemDelegate.computeSelectedDevs())
                        }
                        Button {
                            text: App.hasDir(modelData.dirId) ? qsTr("Share existing folder") : qsTr("Share new folder")
                            flat: true
                            onClicked: page.shareFolder(modelData.dirId, itemDelegate.computeSelectedDevs())
                        }
                    }
                }
            }
            required property var modelData
            function computeSelectedDevs() {
                const devs = {};
                const devModel = modelData.devs;
                const devCount = devModel.count;
                for (let i = 0; i !== devCount; ++i) {
                    const dev = devModel.get(i);
                    if (dev.checked) {
                        devs[dev.devId] = Object.assign({}, dev);
                    }
                }
                return devs;
            }
        }
        model: ListModel {
            id: listModel
        }
    }

    required property var pages
    required property list<Action> actions

    function load() {
        App.requestFromSyncthing("GET", "cluster/pending/folders", {}, (res, error) => {
            listModel.clear();
            if (error.length > 0) {
                return;
            }
            Object.entries(res).forEach((pendingDir) => {
                const dirId = pendingDir[0];
                let index = 0;
                const devs = Object.entries(pendingDir[1].offeredBy).map((device) => {
                    return {devId: device[0], info: device[1], checked: false, index: index++};
                });
                listModel.append({dirId: dirId, devs: devs});
            });
        });
    }

    function ignoreDir(dirId, selectedDevs) {
        const cfg = App.connection.rawConfig;
        const devs = cfg.devices;
        if (!Array.isArray(devs)) {
            return false;
        }
        for (const dev of devs) {
            const pendingFolder = selectedDevs[dev.deviceID]?.info;
            if (typeof pendingFolder !== "object") {
                continue;
            }
            const ignoredFolders = dev.ignoredFolders;
            if (!Array.isArray(ignoredFolders)) {
                continue;
            }
            ignoredFolders.push({id: dirId, label: pendingFolder.Label ?? pendingFolder.label ?? "", time: new Date().toISOString()});
        }
        App.postSyncthingConfig(cfg, (error) => {
            if (error.length === 0) {
                page.load();
            }
        });
        return true;
    }

    function labelForDir(dirId, selectedDevs) {
        let label = dirId;
        for (const dev of Object.values(selectedDevs)) {
            const devInfo = dev.info;
            const labelFromDevInfo = devInfo.label ?? devInfo.Label ?? "";
            if (labelFromDevInfo.length > 0) {
                label = labelFromDevInfo;
                break;
            }
        }
        return label;
    }

    function shareFolder(dirId, selectedDevs) {
        pages.addDir(dirId, labelForDir(dirId, selectedDevs), Object.keys(selectedDevs), App.hasDir(dirId));
    }
}
