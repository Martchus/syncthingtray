import QtQuick

AdvancedDevConfigPage {
    id: devConfigPage
    title: existing && devName.length > 0 ? qsTr("Config of device \"%1\"").arg(devName) : qsTr("Add new device")
    isDangerous: false
    specialEntriesOnly: true
    specialEntries: [
        {key: "deviceID", label: qsTr("Device ID"), enabled: !devConfigPage.configObjectExists, type: "deviceid", helpUrl: "https://docs.syncthing.net/dev/device-ids.html#device-ids"},
        {key: "name", label: qsTr("Device Name"), desc: qsTr("Shown instead of Device ID. Will be updated to the name the device advertises if left empty.")},
        {key: "paused", label: qsTr("Paused"), desc: qsTr("Whether synchronization with this devices is (temporarily) suspended.")},
        {key: "introducer", label: qsTr("Introducer"), desc: qsTr("Add devices from the introducer to our device list, for mutually shared folders.")},
        {key: "autoAcceptFolders", label: qsTr("Auto Accept"), desc: qsTr("Automatically create or share folders that this device advertises at the default path.")},
        {key: "addresses", label: qsTr("Addresses"), itemLabel: qsTr("Address"), desc: qsTr("Add addresses (e.g. \"tcp://ip:port\", \"tcp://host:port\") or \"dynamic\" to perform automatic discovery of the address."), helpUrl: "https://docs.syncthing.net/users/config#listen-addresses"},
        {key: "numConnections", label: qsTr("Number of Connections"), desc: qsTr("When set to more than one on both devices, Syncthing will attempt to establish multiple concurrent connections. If the values differ, the highest will be used. Set to zero to let Syncthing decide."), helpUrl: "https://docs.syncthing.net/advanced/device-numconnections"},
        {key: "untrusted", label: qsTr("Untrusted"), desc: qsTr("All folders shared with this device must be protected by a password, such that all sent data is unreadable without the given password.")},
        {key: "compression", label: qsTr("Compression"), type: "options", desc: qsTr("Whether to use protocol compression when sending messages to this device."), options: [
            {value: "metadata", label: qsTr("Metadata Only"), desc: qsTr("Compress metadata packets, such as index information. Metadata is usually very compression friendly so this is a good default.")},
            {value: "always", label: qsTr("All data"), desc: qsTr("Compress all packets, including file data. This is recommended if the folders contents are mainly compressible data such as documents or text files.")},
            {value: "never", label: qsTr("Off"), desc: qsTr("Disable all compression.")},
        ]},
        {key: "maxRecvKbps", label: qsTr("Incoming Rate Limit (KiB/s)"), desc: qsTr("Maximum receive rate to use for this device.")},
        {key: "maxSendKbps", label: qsTr("Outgoing Rate Limit (KiB/s)"), desc: qsTr("Maximum send rate to use for this device.")},
        {key: "ignoredFolders", label: qsTr("Ignored folders"), itemLabel: qsTr("Ignored folder without ID/label"), desc: qsTr("The list of the folders that should be ignored. These folders will always be skipped when advertised from this remote device, i.e. they will be logged, but there will be no dialog shown."), helpUrl: "https://docs.syncthing.net/users/config#config-option-device.ignoredfolder"},
    ]
    specialEntriesByKey: ({
        "ignoredFolders.*": [
            {key: "id", label: qsTr("Folder ID"), desc: qsTr("The ID of the folder to be ignored.")},
            {key: "label", label: qsTr("Folder Label"), desc: qsTr("The label of the folder being ignored (for informative purposes).")},
            {key: "time", label: qsTr("Time"), init: () => new Date().toISOString(), desc: qsTr("The time when this entry was added (for informative purposes).")},
        ]
    })
    property bool existing: true
}
