import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

ObjectConfigPage {
    id: advancedConfigPage
    isDangerous: true
    Component.onCompleted: configObject = findConfigObject()
    actions: [
        Action {
            text: qsTr("Apply")
            icon.source: App.faUrlBase + "check"
            onTriggered: advancedConfigPage.applyChanges()
        },
        Action {
            text: qsTr("Remove")
            icon.source: App.faUrlBase + "trash-o"
            enabled: advancedConfigPage.configObjectExists
            onTriggered: removeDialog.open()
        }
    ]

    CustomDialog {
        id: removeDialog
        title: qsTr("Remove %1").arg(advancedConfigPage.entryName)
        contentItem: ColumnLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Do you really want to remove the %1?").arg(advancedConfigPage.entryName)
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("This will only remove the %1 from Syncthing. No files will be deleted on disk.").arg(advancedConfigPage.entryName)
                wrapMode: Text.WordWrap
            }
        }
        onAccepted: advancedConfigPage.removeConfigObject()
    }

    required property string entryName
    required property string entriesKey
    required property var isEntry
    property bool configObjectExists: false
    property bool isNew: false

    function findConfigObject() {
        const cfg = App.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        const entry = Array.isArray(entries) ? entries.find(advancedConfigPage.isEntry) : undefined;
        return (advancedConfigPage.configObjectExists = (entry !== undefined)) ? entry : advancedConfigPage.makeNewConfig();
    }

    function isPresentEmptyString(value) {
        return typeof value === "string" && value.length === 0;
    }

    function applyChanges() {
        const cfg = App.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        if (!Array.isArray(entries)) {
            return false;
        }
        advancedConfigPage.updateIdentification();
        const cfgObj = configObject;
        if (isPresentEmptyString(cfgObj.id) || isPresentEmptyString(cfgObj.deviceID)) {
            App.showError(qsTr("The ID must not be empty."));
            return false;
        }
        const index = entries.findIndex(advancedConfigPage.isEntry);
        const supposedToExist = advancedConfigPage.configObjectExists;
        if (supposedToExist && index >= 0) {
            entries[index] = cfgObj;
        } else if (!supposedToExist && index < 0) {
            entries.push(cfgObj);
        } else {
            App.showError(qsTr("Can't apply, ID is already used."));
            return false;
        }
        App.postSyncthingConfig(cfg, (error) => {
            if (error.length === 0) {
                advancedConfigPage.configObjectExists = true;
                advancedConfigPage.hasUnsavedChanges = false;
                advancedConfigPage.disableInitialProperties();
            }
        });
        return true;
    }

    function removeConfigObject() {
        const cfg = App.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        if (!Array.isArray(entries)) {
            return false;
        }
        const index = entries.findIndex(advancedConfigPage.isEntry);
        if (index < 0) {
            return false;
        }
        entries.splice(index, 1);
        App.postSyncthingConfig(cfg, (error) => {
            advancedConfigPage.configObjectExists = false;
            advancedConfigPage.hasUnsavedChanges = false;
        });
        return true;
    }
}
