import QtQuick
import QtQuick.Controls.Material

import Main

AdvancedConfigPage {
    id: dirConfigPage
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    entryName: qsTr("folder")
    entriesKey: "folders"
    isEntry: (folder) => folder.id === dirId
    configCategory: "config-option-folder"
    componentName: "AdvancedDirConfigPage.qml"
    extraActions: [
        Action {
            text: qsTr("Ignore patterns")
            icon.source: QuickUI.faUrlBase + "filter"
            enabled: dirConfigPage.configObjectExists
            onTriggered: QuickUI.editIgnorePatterns(dirConfigPage.dirId, dirConfigPage.dirName, dirConfigPage.stackView)
        }
    ]
    required property string dirName
    required property string dirId
    function makeNewConfig() {
        const config = SyncthingData.connection.rawConfig?.defaults?.folder;
        if (typeof config !== "object" || config.id === undefined) {
            return undefined;
        }

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
