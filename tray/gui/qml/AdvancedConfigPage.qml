import QtQuick
import QtQuick.Controls

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
            onTriggered: (source) => {
                const cfg = app.connection.rawConfig;
                const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
                if (!Array.isArray(entries)) {
                    return false;
                }
                const index = entries.findIndex(advancedConfigPage.isEntry);
                if (index >= 0) {
                    entries[index] = configObject;
                } else {
                    entries.push(configObject);
                }

                app.connection.postConfigFromJsonObject(cfg);
                return true;
            }
        }
    ]
    required property string entriesKey
    required property var isEntry

    function findConfigObject() {
        const cfg = app.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        const entry = Array.isArray(entries) ? entries.find(advancedConfigPage.isEntry) : undefined;
        return entry !== undefined ? entry : {};
    }
}
