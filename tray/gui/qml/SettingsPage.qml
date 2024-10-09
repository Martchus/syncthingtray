import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        id: appSettingsPage
        title: qsTr("App settings")
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: listView
            anchors.fill: parent
            model: ListModel {
                id: model
                ListElement {
                    key: "connection"
                    label: qsTr("Connection to Syncthing backend")
                    title: qsTr("Configure connection with Syncthing backend")
                    iconName: "link"
                }
                ListElement {
                    functionName: "importSettings"
                    label: qsTr("Import settings/data of app and backend, including secrets")
                    iconName: "download"
                }
                ListElement {
                    functionName: "exportSettings"
                    label: qsTr("Export settings/data of app and backend, including secrets")
                    iconName: "floppy-o"
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                //icon.source: app.faUrlBase + iconName // leads to crash when closing UI
                onClicked: key.length === 0 ? appSettingsPage.initiateBackup(functionName) : stackView.push("ObjectConfigPage.qml", {title: title, configObject: appSettingsPage.config[key], specialEntries: appSettingsPage.specialEntries[key] ?? [], stackView: stackView}, StackView.PushTransition)
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        FolderDialog {
            id: backupFolderDialog
            onAccepted: app[appSettingsPage.currentBackupFunction](backupFolderDialog.selectedFolder)
        }

        property string currentBackupFunction
        function initiateBackup(functionName) {
            appSettingsPage.currentBackupFunction = functionName;
            backupFolderDialog.open();
        }

        property var config: app.settings
        readonly property var specialEntries: ({
            connection: [
                {key: "syncthingUrl", label: qsTr("Syncthing URL")},
                {key: "apiKey", label: qsTr("API key")},
                {key: "httpsCertPath", label: qsTr("HTTPs certificate path"), type: "filepath"},
                {key: "httpAuth", label: qsTr("HTTP authentication")},
            ]
        })
        property list<Action> actions: [
            Action {
                text: qsTr("Apply")
                icon.source: app.faUrlBase + "check"
                onTriggered: (source) => {
                    const cfg = app.settings;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        cfg[entryKey] = appSettingsPage.config[entryKey]
                    }
                    app.settings = cfg
                    return true;
                }
            }
        ]
    }
}
