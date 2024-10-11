import QtQuick

AdvancedDirConfigPage {
    title: qsTr("Config of folder \"%1\"").arg(dirName)
    specialEntriesOnly: true
    specialEntries: [
        {key: "id", label: qsTr("ID")},
        {key: "label", label: qsTr("Label")},
        {key: "paused", label: qsTr("Paused")},
        {key: "path", label: qsTr("Path"), type: "folderpath"},
        {key: "type", label: qsTr("Type")},
        {key: "devices", label: qsTr("Shared with")},
    ]
}
