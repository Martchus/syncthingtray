import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
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
                    key: "gui"
                    specialEntriesKey: "guiAuth"
                    label: qsTr("Web-based GUI authentication")
                    iconName: "key-modern"
                }
                ListElement {
                    specialPage: "PendingDevices.qml"
                    label: qsTr("Pending devices")
                    iconName: "laptop"
                }
                ListElement {
                    specialPage: "PendingDirs.qml"
                    label: qsTr("Pending folders")
                    iconName: "folder"
                }
                ListElement {
                    key: "remoteIgnoredDevices"
                    label: qsTr("Ignored devices")
                    title: qsTr("Ignored devices")
                    itemLabel: qsTr("Ignored device without ID/name")
                    desc: qsTr("Contains the IDs of the devices that should be ignored. Connection attempts from these devices are logged to the console but never displayed in the UI.")
                    isDangerous: false
                    helpUrl: "https://docs.syncthing.net/users/config#config-option-configuration.remoteignoreddevice"
                    iconName: "filter"
                }
                ListElement {
                    key: "gui"
                    label: qsTr("Syncthing API and web-based GUI")
                    title: qsTr("Advanced Syncthing API and web-based GUI configuration")
                    isDangerous: true
                    iconName: "cogs"
                }
                ListElement {
                    key: "options"
                    label: qsTr("Various options")
                    title: qsTr("Various advanced options")
                    isDangerous: true
                    iconName: "toggle-on"
                }
                ListElement {
                    key: "defaults"
                    label: qsTr("Templates for new devices and folders")
                    title: qsTr("Templates configuration")
                    isDangerous: true
                    iconName: "puzzle-piece"
                }
                ListElement {
                    key: "ldap"
                    label: qsTr("LDAP")
                    title: qsTr("LDAP configuration")
                    isDangerous: false
                    iconName: "address-card"
                }
                ListElement {
                    label: qsTr("Open config file externally")
                    func: "openSyncthingConfigFile"
                    iconName: "external-link"
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                icon.source: App.faUrlBase + iconName
                icon.width: App.iconSize
                icon.height: App.iconSize
                onClicked: {
                    if (specialPage.length > 0) {
                        stackView.push(specialPage, {pages: stackView.pages}, StackView.PushTransition);
                    } else if (func.length > 0) {
                        App[func]();
                    } else {
                        const specialEntriesOnly = specialEntriesKey.length > 0;
                        const se = (specialEntriesOnly ? advancedPage.specialEntries[specialEntriesKey] : advancedPage.specialEntriesByKey[key]) ?? [];
                        const pageTitle = title.length > 0 ? title : label
                        stackView.push("ObjectConfigPage.qml", {title: pageTitle, isDangerous: isDangerous, configObject: advancedPage.config[key], specialEntries: se, specialEntriesByKey: advancedPage.specialEntriesByKey, specialEntriesOnly: specialEntriesOnly, path: key, configCategory: `config-option-${key}`, itemLabel: itemLabel, helpUrl: helpUrl, stackView: stackView, parentPage: advancedPage, actions: [discardAction, applyAction]}, StackView.PushTransition);
                    }
                }
            }
        }

        property var config: App.connection.rawConfig
        property bool hasUnsavedChanges: false
        property bool isDangerous: false
        readonly property string usernameDesc: qsTr("Set to require authentication for accessing the web-based GUI.")
        readonly property string passwordDesc: qsTr("Contains the bcrypt hash of the password used to restrict accessing the web-based GUI. You can also enter a plain password which will then be hashed when applying the configuration.")
        property var specialEntries: ({
            "guiAuth": [
                {key: "user", label: qsTr("Username"), desc: advancedPage.usernameDesc},
                {key: "password", label: qsTr("Password (turned into bcrypt hash when saving)"), desc: advancedPage.passwordDesc},
            ],
        })
        property var specialEntriesByKey: ({
            "gui": [
                {key: "apiKey", label: qsTr("API Key"), inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase, desc: qsTr("If set, this is the API key that enables usage of the REST interface. The app uses the REST interface so this value must not be empty for the app to function.")},
                {key: "address", label: qsTr("GUI Listen Address"), desc: qsTr("Set the listen address.")},
                {key: "user", label: qsTr("GUI Authentication User"), desc: advancedPage.usernameDesc},
                {key: "password", label: qsTr("GUI Authentication Password (bcrypt hash!)"), inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase, desc: advancedPage.passwordDesc},
                {key: "useTLS", label: qsTr("Use HTTPS for GUI and API"), desc: qsTr("If enabled, TLS (HTTPS) will be enforced. Non-HTTPS requests will be redirected to HTTPS. When set to false, TLS connections are still possible but not required.")},
                {key: "sendBasicAuthPrompt", label: qsTr("Prompt for basic authentication"), desc: qsTr("When this setting is enabled, the web-based GUI will respond to unauthenticated requests with a 401 response prompting for Basic Authorization, so that https://user:pass@localhost style URLs continue to work in standard browsers. Other clients that always send the Authorization request header do not need this setting.")},
                {key: "authMode", label: qsTr("Authentication mode"), type: "options", desc: qsTr("Authentication mode to use. If not present, the authentication mode (static) is controlled by the presence of user/password fields for backward compatibility."), options: [
                    {value: "static", label: "Static", desc: qsTr("Authentication using user and password.")},
                    {value: "ldap", label: "LDAP", desc: qsTr("LDAP authentication. Requires ldap top level config section to be present.")},
                ]},
                {key: "theme", type: "options", label: qsTr("Theme of web-based GUI"), desc: qsTr("The name of the theme to use."), options: [
                    {value: "default", label: "Default"},
                    {value: "light", label: "Light"},
                    {value: "dark", label: "Dark"},
                    {value: "black", label: "Black"},
                ]},
                {key: "debugging", label: qsTr("Profiling and Debugging"), desc: qsTr("This enables Profiling and additional endpoints in the REST API.")},
                {key: "enabled", label: qsTr("Enabled"), desc: qsTr("If disabled, the GUI and API will not be started. The app needs this to function.")},
            ],
            "options": [
                {key: "auditEnabled", label: qsTr("Audit Log"), desc: qsTr("Write events to timestamped file `audit-YYYYMMDD-HHMMSS.log` within the Syncthing home directory. The path can be overidden via \"Audit File\".")},
                {key: "auditFile", type: "filepath", fileMode: FileDialog.SaveFile, label: qsTr("Audit File"), desc: qsTr("Path to store audit events under if \"Audit Log\" is enabled.")},
                {key: "listenAddresses", label: qsTr("Sync Protocol Listen Addresses"), itemLabel: qsTr("Address"), desc: qsTr("Specifies one or more listen addresses for the sync protocol. Set to default to listen on port TCP and QUIC port 22000.")},
                {key: "maxRecvKbps", label: qsTr("Incoming Rate Limit (KiB/s)"), desc: qsTr("Incoming data rate limits, in kibibytes per second.")},
                {key: "maxSendKbps", label: qsTr("Outgoing Rate Limit (KiB/s)"), desc: qsTr("Outgoing data rate limit, in kibibytes per second.")},
                {key: "limitBandwidthInLan", label: qsTr("Limit Bandwidth in LAN"), desc: qsTr("Whether to apply bandwidth limits to devices in the same broadcast domain as the local device.")},
                {key: "natEnabled", label: qsTr("NAT traversal"), desc: qsTr("Whether to attempt to perform a UPnP and NAT-PMP port mapping for incoming sync connections.")},
                {key: "localAnnounceEnabled", label: qsTr("Local Discovery"), desc: qsTr("Whether to send announcements to the local LAN, also use such announcements to find other devices.")},
                {key: "globalAnnounceEnabled", label: qsTr("Global Discovery"), desc: qsTr("Whether to announce this device to the global announce (discovery) server, and also use it to look up other devices.")},
                {key: "globalAnnounceServers", label: qsTr("Global Discovery Servers"), itemLabel: qsTr("URI"), desc: qsTr("A URI to a global announce (discovery) server, or the word \"default\" to include the default servers. Multiple servers can be added. The syntax for non-default entries is that of an HTTP or HTTPS URL. A number of options may be added as query options to the URL: insecure to prevent certificate validation (required for HTTP URLs) and \"id=<device ID>\" to perform certificate pinning. The device ID to use is printed by the discovery server on startup.")},
                {key: "relaysEnabled", label: qsTr("Relaying"), desc: qsTr("Whether relays will be connected to and potentially used for device to device connections.")},
                {key: "minHomeDiskFree", label: qsTr("Minimum Free Space (Home)"), desc: qsTr("The minimum required free space that should be available on the partition holding the configuration and index. The element content is interpreted according to the given unit attribute. Accepted unit values are \"%\" (percent of the disk / volume size), kB, MB, GB and TB. Set to zero to disable.")},
                {key: "announceLANAddresses", label: qsTr("Announce LAN Addresses"), desc: qsTr("Enable (the default) or disable announcing private (RFC1918) LAN IP addresses to global discovery.")},
                {key: "alwaysLocalNets", label: qsTr("Networks to consider always local"), itemLabel: qsTr("Network in CIDR notation"), desc: qsTr("Network that should be considered as local given in CIDR notation.")},
            ],
            "options.minHomeDiskFree": [
               {key: "value", label: qsTr("Value"), desc: qsTr("The minimum required free space that should be available on the partition holding the configuration and index. The element content is interpreted according to the given unit attribute. Accepted unit values are \"%\" (percent of the disk / volume size), kB, MB, GB and TB. Set to zero to disable.")},
               {key: "unit", label: qsTr("Unit"), type: "options", options: [
                   {value: "%", label: qsTr("Percent"), desc: qsTr("Percentage of the disk/volume size")},
                   {value: "kB", label: qsTr("Kilobyte"), desc: qsTr("Absolute size in Kilobyte")},
                   {value: "MB", label: qsTr("Megabyte"), desc: qsTr("Absolute size in Megabyte")},
                   {value: "GB", label: qsTr("Gigabyte"), desc: qsTr("Absolute size in Gigabyte")},
                   {value: "TB", label: qsTr("Terrabyte"), desc: qsTr("Absolute size in Terrabyte")},
               ]},
            ],
        })
        property list<Action> actions: [
            Action {
                id: discardAction
                text: qsTr("Discard changes")
                icon.source: App.faUrlBase + "undo"
                enabled: advancedPage.hasUnsavedChanges
                onTriggered: {
                    advancedPage.config = App.connection.rawConfig;
                    advancedPage.hasUnsavedChanges = false;
                    advancedPage.isDangerous = false;
                }
            },
            Action {
                id: applyAction
                text: qsTr("Apply changes")
                icon.source: App.faUrlBase + "check"
                enabled: advancedPage.hasUnsavedChanges
                onTriggered: {
                    const cfg = App.connection.rawConfig;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        cfg[entryKey] = advancedPage.config[entryKey]
                    }
                    App.postSyncthingConfig(cfg, (error) => {
                        if (error.length === 0) {
                            const currentPage = stackView.currentItem;
                            if (currentPage) {
                                currentPage.hasUnsavedChanges = false;
                            }
                            advancedPage.hasUnsavedChanges = false;
                            advancedPage.isDangerous = false;
                        }
                    });
                    return true;
                }
            }
        ]
    }
    function showGuiAuth() {
        advancedPage.hasUnsavedChanges = true;
        listView.currentIndex = 0;
        listView.currentItem.click();
    }
    required property var pages
}
