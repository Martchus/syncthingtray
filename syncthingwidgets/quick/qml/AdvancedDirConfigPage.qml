import QtQuick

import Main

AdvancedConfigPage {
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    entryName: qsTr("folder")
    entriesKey: "folders"
    isEntry: (folder) => folder.id === dirId
    configCategory: "config-option-folder"
    required property string dirName
    required property string dirId
    function makeNewConfig() {
        const config = App.connection.rawConfig?.defaults?.folder ?? {};

        // for now, give user simply always the chance to edit ignore patterns before syncing
        config.paused = true;

        // add dirId/dirName as default values for id/label
        if (dirId.length > 0) {
            config.id = dirId;
        }
        if (dirName.length > 0) {
            config.label = dirName;
        }

        isNew = true;
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
