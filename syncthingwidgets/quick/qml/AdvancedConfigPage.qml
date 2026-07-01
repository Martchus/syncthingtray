import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

ObjectConfigPage {
    id: advancedConfigPage
    isDangerous: true
    componentName: "AdvancedConfigPage.qml"
    isLoading: !advancedConfigPage.configObjectInitialized
    Component.onCompleted: initConfigObject()
    actions: [
        Action {
            text: qsTr("Apply")
            icon.source: QuickUI.faUrlBase + "check"
            onTriggered: advancedConfigPage.applyChanges()
        },
        Action {
            text: qsTr("Remove")
            icon.source: QuickUI.faUrlBase + "trash-o"
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
    property bool configObjectInitialized: false
    property bool configObjectExists: false
    property bool isNew: false
    property Connections connectionConnections: Connections {
        target: SyncthingData.connection
        function onNewConfig() {
            advancedConfigPage.initConfigObject() && advancedConfigPage.reloadEntries();
        }
    }

    function initConfigObject() {
        if (advancedConfigPage.configObjectInitialized) {
            return false;
        }
        const cfg = SyncthingData.connection.rawConfig;
        const entries = cfg?.[entriesKey];
        if (!Array.isArray(entries)) {
            return false;
        }
        const existingEntry = entries.find(advancedConfigPage.isEntry);
        const entry = existingEntry ?? advancedConfigPage.makeNewConfig();
        advancedConfigPage.configObjectExists = existingEntry !== undefined;
        advancedConfigPage.configObject = entry;
        advancedConfigPage.configObjectInitialized = entry !== undefined;
        return true;
    }

    function isPresentEmptyString(value) {
        return typeof value === "string" && value.length === 0;
    }

    function applyChanges() {
        const cfg = SyncthingData.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        if (!Array.isArray(entries)) {
            return false;
        }
        advancedConfigPage.updateIdentification();
        const cfgObj = configObject;
        if (isPresentEmptyString(cfgObj.id) || isPresentEmptyString(cfgObj.deviceID)) {
            QuickUI.showError(qsTr("The ID must not be empty."));
            return false;
        }
        const index = entries.findIndex(advancedConfigPage.isEntry);
        const supposedToExist = advancedConfigPage.configObjectExists;
        if (supposedToExist && index >= 0) {
            entries[index] = cfgObj;
        } else if (!supposedToExist && index < 0) {
            entries.push(cfgObj);
        } else {
            QuickUI.showError(qsTr("Can't apply, ID is already used."));
            return false;
        }
        SyncthingModels.postSyncthingConfig(cfg, (error) => {
            if (error.length === 0) {
                advancedConfigPage.configObjectExists = true;
                advancedConfigPage.hasUnsavedChanges = false;
                advancedConfigPage.disableInitialProperties();
            }
        });
        return true;
    }

    function removeConfigObject() {
        const cfg = SyncthingData.connection.rawConfig;
        const entries = cfg !== undefined ? cfg[entriesKey] : undefined;
        if (!Array.isArray(entries)) {
            return false;
        }
        const index = entries.findIndex(advancedConfigPage.isEntry);
        if (index < 0) {
            return false;
        }
        entries.splice(index, 1);
        SyncthingModels.postSyncthingConfig(cfg, (error) => {
            advancedConfigPage.configObjectExists = false;
            advancedConfigPage.hasUnsavedChanges = false;
        });
        return true;
    }
}
