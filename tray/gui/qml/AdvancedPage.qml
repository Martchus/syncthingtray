import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        id: advancedPage
        title: qsTr("Advanced")
        Layout.fillWidth: true
        Layout.fillHeight: true

        CustomListView {
            id: listView
            anchors.fill: parent
            model: ListModel {
                id: model
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
                onClicked: stackView.push("ObjectConfigPage.qml", {title: title, isDangerous: isDangerous, configObject: advancedPage.config[key], path: key, configCategory: `config-option-${key}`, itemLabel: itemLabel, helpUrl: helpUrl, stackView: stackView}, StackView.PushTransition)
            }
        }

        property var config: App.connection.rawConfig
        property list<Action> actions: [
            Action {
                text: qsTr("Apply")
                icon.source: App.faUrlBase + "check"
                onTriggered: (source) => {
                    const cfg = App.connection.rawConfig;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        cfg[entryKey] = advancedPage.config[entryKey]
                    }
                    App.postSyncthingConfig(cfg);
                    return true;
                }
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Restart Syncthing")
                icon.source: App.faUrlBase + "refresh"
                onTriggered: (source) => App.connection.restart()
            },
            Action {
                text: qsTr("Shutdown Syncthing")
                icon.source: App.faUrlBase + "power-off"
                onTriggered: (source) => App.connection.shutdown()
            }
        ]
    }
}
