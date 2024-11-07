import QtQuick

import Main

AdvancedConfigPage {
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    entryName: "folder"
    entriesKey: "folders"
    isEntry: (folder) => folder.id === dirId
    configCategory: "config-option-folder"
    Component.onCompleted: configObject = dirId.length > 0 ? findConfigObject() : makeNewConfig();
    required property string dirName
    required property string dirId
    function makeNewConfig() {
        const config = App.connection.rawConfig?.defaults?.folder ?? {};
        // for now, give user simply always the chance to edit ignore patterns before syncing
        config.paused = true;
        return config;
    }
    function updateIdentification() {
        const id = configObject.id ?? "";
        const label = configObject.label ?? "";
        dirName = label.length > 0 ? label : id;
        if (!configObjectExists) {
            dirId = id;
        }
    }
}
