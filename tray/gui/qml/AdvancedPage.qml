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

        ListView {
            id: listView
            anchors.fill: parent
            model: ListModel {
                id: model
                ListElement {
                    key: "gui"
                    label: qsTr("Syncthing API and web-based GUI")
                    title: qsTr("Advanced Syncthing API and GUI configuration")
                }
                ListElement {
                    key: "options"
                    label: qsTr("Various options")
                    title: qsTr("Various advanced options")
                }
                ListElement {
                    key: "defaults"
                    label: qsTr("Templates for new devices and folders")
                    title: qsTr("Templates configuration")
                }
                ListElement {
                    key: "ldap"
                    label: qsTr("LDAP")
                    title: qsTr("LDAP configuration")
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                onClicked: stackView.push("ObjectConfigPage.qml", {title: title, isDangerous: true, configObject: advancedPage.config[key], configCategory: `config-option-${key}`, stackView: stackView}, StackView.PushTransition)
            }
            ScrollIndicator.vertical: ScrollIndicator { }
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
                    App.connection.postConfigFromJsonObject(cfg);
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
