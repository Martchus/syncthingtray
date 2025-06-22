import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

Page {
    id: importPage
    title: qsTr("Select settings to import")
    Layout.fillWidth: true
    Layout.fillHeight: true
    CustomFlickable {
        id: mainView
        anchors.fill: parent
        contentHeight: mainLayout.height
        ColumnLayout {
            id: mainLayout
            width: mainView.width
            spacing: 0
            ItemDelegate {
                Layout.fillWidth: true
                visible: importPage.availableSettings.error !== undefined
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "exclamation-circle"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("An error occurred when checking selected directory")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            Layout.fillWidth: true
                            text: importPage.availableSettings.error ?? ""
                            elide: Text.ElideRight
                            font.weight: Font.Light
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
            ItemDelegate {
                Layout.fillWidth: true
                enabled: availableSettings.appConfigPath !== undefined
                onClicked: appConfig.toggle()
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "cog"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("App configuration")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Replace the app configuration with the one from the selected directory.")
                            elide: Text.ElideRight
                            font.weight: Font.Light
                            wrapMode: Text.WordWrap
                        }
                    }
                    Switch {
                        id: appConfig
                    }
                }
            }
            ItemDelegate {
                id: fullImportDelegate
                Layout.fillWidth: true
                enabled: availableSettings.syncthingHomePath !== undefined
                onClicked: {
                    fullImport.toggle();
                    fullImport.toggled();
                }
                contentItem: RowLayout {
                    spacing: 15
                    ForkAwesomeIcon {
                        iconName: "syncthing"
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Full Syncthing configuration and database")
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                            wrapMode: Text.WordWrap
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Replace entire (existing) Syncthing configuration and database with the one from the selected directory. Use this with care as restoring the database is potentially dangerous.")
                            elide: Text.ElideRight
                            font.weight: Font.Light
                            wrapMode: Text.WordWrap
                        }
                    }
                    Switch {
                        id: fullImport
                        onToggled: {
                            if (fullImport.checked) {
                                folderSelection.selectionEnabled = false;
                                deviceSelection.selectionEnabled = false;
                            }
                        }
                    }
                }
            }
            SelectiveImportDelegate {
                id: folderSelection
                enabled: availableSettings.folders !== undefined && !fullImport.checked
                iconName: "folder"
                text: qsTr("Selected folders")
                description: qsTr("Merge the selected folders into the existing Syncthing configuration. You can change paths in case they differ on this device.")
                dialogTitle: qsTr("Select folders to import")
                model: ListModel {
                    id: foldersModel
                    Component.onCompleted: {
                        const folders = importPage.availableSettings.folders;
                        if (Array.isArray(folders)) {
                            folders.forEach((folder, index) => foldersModel.append({index: index, displayName: folder.label?.length > 0 ? folder.label : folder.id, path: folder.path, checked: false}));
                        }
                    }
                }
            }
            SelectiveImportDelegate {
                id: deviceSelection
                enabled: availableSettings.devices !== undefined && !fullImport.checked
                iconName: "sitemap"
                text: qsTr("Selected devices")
                description: qsTr("Merge the selected devices into the existing Syncthing configuration.")
                dialogTitle: qsTr("Select devices to import")
                model: ListModel {
                    id: devicesModel
                    Component.onCompleted: {
                        const devices = importPage.availableSettings.devices;
                        if (Array.isArray(devices)) {
                            devices.forEach((device, index) => devicesModel.append({index: index, displayName: device.name?.length > 0 ? `${device.name}\n${device.deviceID}` : device.deviceID, checked: false}));
                        }
                    }
                }
            }
        }
    }
    required property var availableSettings
    readonly property bool isDangerous: fullImport.checked
    property var selectedConfig: ({
        appConfig: appConfig.checked,
        syncthingHome: fullImport.checked,
        selectedFolders: folderSelection.selectionEnabled ? handleSelectedIndexes(foldersModel) : [],
        selectedDevices: deviceSelection.selectionEnabled ? handleSelectedIndexes(devicesModel) : [],
    })
    property list<Action> actions: [
        Action {
            text: qsTr("Import selected")
            icon.source: App.faUrlBase + "download"
            onTriggered: App.importSettings(importPage.availableSettings, importPage.selectedConfig)
        }
    ]
    function back() {
        App.importSettings(importPage.availableSettings, {aborted: true});
        return false;
    }
    function handleSelectedIndexes(model) {
        const indexes = [];
        const folders = importPage.availableSettings.folders;
        for (let i = 0, count = model.count; i !== count; ++i) {
            const modelData = model.get(i);
            if (modelData.checked) {
                // update paths
                const path = modelData.path;
                if (path !== undefined && Array.isArray(folders) && i < folders.length) {
                    folders[i].path = path;
                }
                indexes.push(i);
            }
        }
        return indexes;
    }
}
