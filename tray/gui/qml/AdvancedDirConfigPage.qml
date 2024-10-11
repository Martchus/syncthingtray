import QtQuick

AdvancedConfigPage {
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    entriesKey: "folders"
    isEntry: (folder) => folder.id === dirId
    configObject: dirId.length > 0 ? findConfigObject() : makeNewConfig()
    configCategory: "config-option-folder"
    required property string dirName
    required property string dirId
    function makeNewConfig() {
        const config = app.connection.rawConfig?.defaults?.folder ?? {};
        // for now, give user simply always the chance to edit ignore patterns before syncing
        config.paused = true;
        return config;
    }
}
