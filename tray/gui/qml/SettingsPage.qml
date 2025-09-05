import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        id: appSettingsPage
        title: qsTr("App settings")
        Layout.fillWidth: true
        Layout.fillHeight: true

        CustomListView {
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
                    callback: () => deleteLogFileDialog.open()
                    label: qsTr("Clear log file")
                    title: qsTr("Disables persistent logging and removes the log file")
                    iconName: "trash-o"
                }
                ListElement {
                    key: "tweaks"
                    label: qsTr("Tweaks")
                    title: qsTr("Configure details of the app's behavior")
                    iconName: "cogs"
                }
                ListElement {
                    callback: () => stackView.push("ErrorsPage.qml", {}, StackView.PushTransition)
                    label: qsTr("Syncthing notifications/errors")
                    iconName: "exclamation-triangle"
                }
                ListElement {
                    callback: () => stackView.push("InternalErrorsPage.qml", {}, StackView.PushTransition)
                    label: qsTr("Log of Syncthing API errors")
                    iconName: "exclamation-circle"
                }
                ListElement {
                    callback: () => stackView.push("StatisticsPage.qml", {stackView: stackView}, StackView.PushTransition)
                    label: qsTr("Statistics")
                    iconName: "area-chart"
                }
                ListElement {
                    functionName: "checkSettings"
                    callback: (availableSettings) => stackView.push("ImportPage.qml", {availableSettings: availableSettings}, StackView.PushTransition)
                    label: qsTr("Import selected settings/secrets/data of app and backend")
                    iconName: "download"
                }
                ListElement {
                    functionName: "exportSettings"
                    label: qsTr("Export all settings/secrets/data of app and backend")
                    iconName: "floppy-o"
                }
                ListElement {
                    callback: () => stackView.push("HomeDirPage.qml", {}, StackView.PushTransition)
                    label: qsTr("Move Syncthing home directory")
                    iconName: "folder-open-o"
                }
                ListElement {
                    callback: () => App.cleanSyncthingHomeDirectory()
                    label: qsTr("Clean Syncthing home directory")
                    title: qsTr("Removes the migrated database of Syncthing v1")
                    iconName: "eraser"
                }
                ListElement {
                    label: qsTr("Save support bundle")
                    functionName: "saveSupportBundle"
                    iconName: "user-md"
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                icon.source: App.faUrlBase + iconName // leads to crash when closing UI with Qt < 6.8
                icon.width: App.iconSize
                icon.height: App.iconSize
                onClicked: {
                    if (key.length > 0) {
                        appSettingsPage.openNestedSettings(title, key);
                    } else if (functionName.length > 0) {
                        appSettingsPage.initiateBackup(functionName, callback);
                    } else if (callback !== undefined) {
                        callback();
                    }
                }

            }
        }

        FileDialog {
            id: backupFileDialog
            fileMode: appSettingsPage.currentBackupFunction === "exportSettings" || appSettingsPage.currentBackupFunction === "saveSupportBundle"
                      ? FileDialog.SaveFile : FileDialog.OpenFile
            onAccepted: App[appSettingsPage.currentBackupFunction](backupFileDialog.selectedFile, appSettingsPage.currentBackupCallback)
        }

        FolderDialog {
            id: backupFolderDialog
            onAccepted: App[appSettingsPage.currentBackupFunction](backupFolderDialog.selectedFolder, appSettingsPage.currentBackupCallback)
        }

        CustomDialog {
            id: deleteLogFileDialog
            title: meta.title
            contentItem: Label {
                Layout.fillWidth: true
                text: qsTr("Do you really want to delete the persistent log file?")
                wrapMode: Text.WordWrap
            }
            onAccepted: App.clearLogfile()
        }

        property string currentBackupFunction
        property var currentBackupCallback
        function initiateBackup(functionName, callback) {
            const tweaks = App.settings.tweaks;
            appSettingsPage.currentBackupFunction = functionName;
            appSettingsPage.currentBackupCallback = callback;
            if (tweaks.exportDir?.length > 0 && (functionName === "exportSettings" || functionName === "saveSupportBundle")) {
                return App[functionName]("", callback);
            }
            return tweaks.importExportAsArchive || functionName === "saveSupportBundle" ? backupFileDialog.open() : backupFolderDialog.open();
        }
        function openNestedSettings(title, key) {
            if (appSettingsPage.config[key] === undefined) {
                appSettingsPage.config[key] = {};
            }
            stackView.push("ObjectConfigPage.qml", {
                               title: title,
                               parentPage: appSettingsPage,
                               configObject: Qt.binding(() => appSettingsPage.config[key]),
                               specialEntries: appSettingsPage.specialEntries[key] ?? [],
                               specialEntriesByKey: appSettingsPage.specialEntries,
                               specialEntriesOnly: true,
                               stackView: stackView,
                               actions: appSettingsPage.actions},
                           StackView.PushTransition)
        }

        property var config: App.settings
        readonly property var specialEntries: ({
            connection: [
                {key: "useLauncher", type: "boolean", label: qsTr("Automatic"), statusText: qsTr("Connect to the Syncthing backend launched via this app and disregard the settings below.")},
                {key: "syncthingUrl", label: qsTr("Syncthing URL")},
                {key: "apiKey", label: qsTr("API key"), inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase},
                {key: "httpsCertPath", label: qsTr("HTTPs certificate path"), type: "filepath"},
                {key: "httpAuth", label: qsTr("HTTP authentication")},
            ],
            httpAuth: [
                {key: "enabled", label: qsTr("Enabled")},
                {key: "userName", label: qsTr("Username")},
                {key: "password", label: qsTr("Password"), inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase},
            ],
            launcher: [
                {key: "run", label: qsTr("Run Syncthing"), statusText: Qt.binding(() => App.syncthingRunningStatus)},
                {key: "logLevel", label: qsTr("Log level"), type: "options", options: [
                    {value: "debug", label: qsTr("Debug")},
                    {value: "info", label: qsTr("Info")},
                    {value: "warning", label: qsTr("Warning")},
                    {value: "error", label: qsTr("Error")},
                ]},
                {key: "stopOnMetered", label: qsTr("Stop on metered network connection"), statusText: Qt.binding(() => App.meteredStatus)},
                {key: "writeLogFile", label: qsTr("Write persistent log file"), statusText: qsTr("Write a persistent log file into the app directory")},
                {key: "openLogs", label: qsTr("Open logs"), statusText: qsTr("Show Syncthing logs since app startup"), defaultValue: () => stackView.push("LogPage.qml", {}, StackView.PushTransition)},
                {key: "openPersistentLogs", label: qsTr("Open persistent logs"), statusText: qsTr("Open persistent log file externally"), defaultValue: () => App.openSyncthingLogFile()},
            ],
            tweaks: [
                {key: "importExportAsArchive", type: "boolean", defaultValue: false, label: qsTr("Import/export archive"), statusText: qsTr("Import and export to/from a Zip archive")},
                {key: "importExportEncryptionPassword", type: "string", inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase, defaultValue: "", label: qsTr("Import/export password"), statusText: qsTr("Encrypt/decrypt data via AES-256 when exporting/importing to archive")},
                {key: "exportDir", type: "folderpath", defaultValue: "", label: qsTr("Export path"), statusText: qsTr("Save exports and support bundles under fix location")},
                {key: "useUnixDomainSocket", type: "boolean", defaultValue: false, label: qsTr("Use Unix domain socket"), statusText: qsTr("Reduces communication overhead and makes Syncthing API and web GUI inaccessible to other apps, applied after restart")},
            ]
        })
        property bool hasUnsavedChanges: false
        property list<Action> actions: [
            Action {
                text: qsTr("Apply")
                icon.source: App.faUrlBase + "check"
                onTriggered: {
                    const cfg = App.settings;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        if (entryKey.length > 0) {
                            cfg[entryKey] = appSettingsPage.config[entryKey];
                        }
                    }
                    App.settings = cfg;
                    return true;
                }
            }
        ]
    }
}
