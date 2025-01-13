import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        id: advancedPage
        title: hasUnsavedChanges ? qsTr("Advanced - changes not saved yet") : qsTr("Advanced")
        Layout.fillWidth: true
        Layout.fillHeight: true

        CustomListView {
            id: listView
            anchors.fill: parent
            model: ListModel {
                id: model
                ListElement {
                    specialPage: "PendingDevices.qml"
                    label: qsTr("Pending devices")
                }
                ListElement {
                    specialPage: "PendingDirs.qml"
                    label: qsTr("Pending folders")
                }
                ListElement {
                    key: "remoteIgnoredDevices"
                    label: qsTr("Ignored devices")
                    title: qsTr("Ignored devices")
                    itemLabel: qsTr("Ignored device without ID/name")
                    desc: qsTr("Contains the IDs of the devices that should be ignored. Connection attempts from these devices are logged to the console but never displayed in the UI.")
                    isDangerous: false
                    helpUrl: "https://docs.syncthing.net/users/config#config-option-configuration.remoteignoreddevice"
                }
                ListElement {
                    key: "gui"
                    label: qsTr("Syncthing API and web-based GUI")
                    title: qsTr("Advanced Syncthing API and GUI configuration")
                    isDangerous: true
                }
                ListElement {
                    key: "options"
                    label: qsTr("Various options")
                    title: qsTr("Various advanced options")
                    isDangerous: true
                }
                ListElement {
                    key: "defaults"
                    label: qsTr("Templates for new devices and folders")
                    title: qsTr("Templates configuration")
                    isDangerous: true
                }
                ListElement {
                    key: "ldap"
                    label: qsTr("LDAP")
                    title: qsTr("LDAP configuration")
                    isDangerous: false
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                onClicked: {
                    if (specialPage.length > 0) {
                        stackView.push(specialPage, {pages: stackView.pages}, StackView.PushTransition)
                    } else {
                        stackView.push("ObjectConfigPage.qml", {title: title, isDangerous: isDangerous, configObject: advancedPage.config[key], path: key, configCategory: `config-option-${key}`, itemLabel: itemLabel, helpUrl: helpUrl, stackView: stackView, parentPage: advancedPage}, StackView.PushTransition)
                    }
                }
            }
        }

        property var config: App.connection.rawConfig
        property bool hasUnsavedChanges: false
        property bool isDangerous: false
        property list<Action> actions: [
            Action {
                text: qsTr("Discard changes")
                icon.source: App.faUrlBase + "undo"
                enabled: advancedPage.hasUnsavedChanges
                onTriggered: (source) => {
                    advancedPage.config = App.connection.rawConfig;
                    advancedPage.hasUnsavedChanges = false;
                    advancedPage.isDangerous = false;
                }
            },
            Action {
                text: qsTr("Apply changes")
                icon.source: App.faUrlBase + "check"
                enabled: advancedPage.hasUnsavedChanges
                onTriggered: (source) => {
                    const cfg = App.connection.rawConfig;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        cfg[entryKey] = advancedPage.config[entryKey]
                    }
                    App.postSyncthingConfig(cfg, (error) => {
                        if (error.length === 0) {
                            advancedPage.hasUnsavedChanges = false;
                            advancedPage.isDangerous = false;
                        }
                    });
                    return true;
                }
            }
        ]
    }
    required property var pages
}
