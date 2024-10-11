import QtQuick

AdvancedDevConfigPage {
    title: qsTr("Config of device \"%1\"").arg(devName)
    specialEntriesOnly: true
    specialEntries: [
        {key: "deviceID", label: qsTr("ID")},
        {key: "name", label: qsTr("Name")},
        {key: "paused", label: qsTr("Paused")},
    ]
}
