import QtQuick

AdvancedConfigPage {
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    entriesKey: "folders"
    isEntry: (folder) => folder.id === dirId
    configCategory: "config-option-folder"
    required property string dirName
    required property string dirId
}
