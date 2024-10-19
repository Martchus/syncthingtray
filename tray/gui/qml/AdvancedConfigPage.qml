import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ObjectConfigPage {
    id: advancedConfigPage
    configObject: findConfigObject()
    configTemplates: {
        ".devices": {deviceID: "", introducedBy: "", encryptionPassword: ""}
    }
    actions: [
        Action {
            text: qsTr("Apply")
            icon.source: app.faUrlBase + "check"
            onTriggered: (source) => advancedConfigPage.applyChanges()
        },
        Action {
            text: qsTr("Remove")
            icon.source: app.faUrlBase + "trash-o"
            enabled: advancedConfigPage.configObjectCache !== undefined
            onTriggered: (source) => removeDialog.open()
        }
    ]

    MessageDialog {
        id: removeDialog
        text: qsTr("Do you really want to remove the %1?").arg(advancedConfigPage.entryName)
        informativeText: qsTr("This will only remove the %1 from Syncthing. No files will be deleted on disk.").arg(advancedConfigPage.entryName)
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: advancedConfigPage.removeConfigObject()
    }

    required property string entryName
    required property string entriesKey
    required property var isEntry
    property var configObjectCache: undefined

    function findConfigObject() {
        if (configObjectCache !== undefined) {
            // keep using the same config object, even if dirId or devId changes (unfortunately causes a binding loop)
            return configObjectCache;
        }
        const cfg = app.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        const entry = Array.isArray(entries) ? entries.find(advancedConfigPage.isEntry) : undefined;
        return configObjectCache = (entry !== undefined ? entry : {});
    }

    function applyChanges() {
        const cfg = app.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        if (!Array.isArray(entries)) {
            return false;
        }
        const cfgObj = configObject;
        const index = entries.findIndex(advancedConfigPage.isEntry);
        advancedConfigPage.updateIdentification();
        if (index >= 0) {
            entries[index] = cfgObj;
        } else {
            entries.push(cfgObj);
        }
        app.connection.postConfigFromJsonObject(cfg);
        return true;
    }

    function removeConfigObject() {
        const cfg = app.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        if (!Array.isArray(entries)) {
            return false;
        }
        const index = entries.findIndex(advancedConfigPage.isEntry);
        advancedConfigPage.updateIdentification();
        if (index < 0) {
            return false;
        }
        entries.splice(index, 1);
        app.connection.postConfigFromJsonObject(cfg);
        return true;
    }
}
