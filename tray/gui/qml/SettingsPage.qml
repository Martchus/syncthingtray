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
                    key: "launcher"
                    label: qsTr("Run conditions of Syncthing backend")
                    title: qsTr("Configure when to run Syncthing backend")
                    iconName: "terminal"
                }
                ListElement {
                    key: "tweaks"
                    label: qsTr("Tweaks")
                    title: qsTr("Configure details of the app's behavior")
                    iconName: "cogs"
                }
                ListElement {
                    callback: () => stackView.push("ErrorsPage.qml", {}, StackView.PushTransition)
                    label: qsTr("Show Syncthing notifications/errors")
                    iconName: "exclamation-triangle"
                }
                ListElement {
                    callback: () => stackView.push("InternalErrorsPage.qml", {}, StackView.PushTransition)
                    label: qsTr("Show log of Syncthing API errors")
                    iconName: "exclamation-circle"
                }
                ListElement {
                    functionName: "importSettings"
                    label: qsTr("Import settings/secrets/data of app and backend")
                    iconName: "download"
                }
                ListElement {
                    functionName: "exportSettings"
                    label: qsTr("Export ettings/secrets/data of app and backend")
                    iconName: "floppy-o"
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                icon.source: app.faUrlBase + iconName // leads to crash when closing UI with Qt < 6.8
                icon.width: app.iconSize
                icon.height: app.iconSize
                onClicked: {
                    if (callback !== undefined) {
                        callback();
                    } else if (key.length > 0) {
                        appSettingsPage.openNestedSettings(title, key);
                    } else {
                        appSettingsPage.initiateBackup(functionName);
                    }
                }

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
        function openNestedSettings(title, key) {
            if (appSettingsPage.config[key] === undefined) {
                appSettingsPage.config[key] = {};
            }
            stackView.push("ObjectConfigPage.qml", {
                               title: title,
                               configObject: Qt.binding(() => appSettingsPage.config[key]),
                               specialEntries: appSettingsPage.specialEntries[key] ?? [],
                               specialEntriesByKey: appSettingsPage.specialEntries,
                               stackView: stackView,
                               actions: appSettingsPage.actions},
                           StackView.PushTransition)
        }

        property var config: app.settings
        readonly property var specialEntries: ({
            connection: [
                {key: "useLauncher", type: "boolean", label: qsTr("Automatic"), statusText: qsTr("Connect to the Syncthing backend launched via this app and disregard the settings below.")},
                {key: "syncthingUrl", label: qsTr("Syncthing URL")},
                {key: "apiKey", label: qsTr("API key")},
                {key: "httpsCertPath", label: qsTr("HTTPs certificate path"), type: "filepath"},
                {key: "httpAuth", label: qsTr("HTTP authentication")},
            ],
            httpAuth: [
                {key: "enabled", label: qsTr("Enabled")},
                {key: "userName", label: qsTr("Username")},
                {key: "password", label: qsTr("Password")},
            ],
            launcher: [
                {key: "run", label: qsTr("Run Syncthing"), statusText: Qt.binding(() => app.launcher.runningStatus)},
                {key: "stopOnMetered", label: qsTr("Stop on metered network connection"), statusText: Qt.binding(() => app.launcher.meteredStatus)},
                {key: "openLogs", label: qsTr("Open logs"), statusText: qsTr("Shows Syncthing logs since app startup"), defaultValue: () => stackView.push("LogPage.qml", {}, StackView.PushTransition)},
            ],
            tweaks: [
                {key: "unloadGuiWhenHidden", type: "boolean", defaultValue: false, label: qsTr("Stop UI when hidden"), statusText: qsTr("Might help save battery live but resets UI state.")},
            ],
        })
        property list<Action> actions: [
            Action {
                text: qsTr("Apply")
                icon.source: app.faUrlBase + "check"
                onTriggered: (source) => {
                    const cfg = app.settings;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        if (entryKey.length > 0) {
                            cfg[entryKey] = appSettingsPage.config[entryKey];
                        }
                    }
                    app.settings = cfg;
                    return true;
                }
            }
        ]
    }
}
