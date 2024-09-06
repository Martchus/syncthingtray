import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ObjectConfigPage {
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    configObject: {
        const folders = app.connection.rawConfig?.folders;
        return Array.isArray(folders) ? folders.find((folder) => folder.id === dirId) : {};
    }
    configCategory: "config-option-folder"
    actions: [
        Action {
            text: qsTr("Apply")
            icon.source: app.faUrlBase + "check"
            onTriggered: (source) => {
                const config = app.connection.rawConfig;
                const folders = config.folders;
                if (!Array.isArray(folders)) {
                    return false;
                }
                const index = folders.findIndex((folder) => folder.id === dirId);
                if (index >= 0) {
                     folders[index] = configObject;
                }
                app.connection.postConfigFromJsonObject(config);
                return true;
            }
        }
    ]
    required property string dirName
    required property string dirId
}
