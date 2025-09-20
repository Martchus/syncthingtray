import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

Page {
    id: objectConfigPage
    StackLayout {
        anchors.fill: parent
        currentIndex: objectConfigPage.isLoading ? 0 : 1
        LoadingPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        CustomListView {
            id: objectListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            cacheBuffer: 10000
            model: ListModel {
                id: listModel
                dynamicRoles: true
                Component.onCompleted: listModel.loadEntries()
                function loadEntries() {
                    const indexByKey = objectConfigPage.indexByKey = {};
                    listModel.clear();

                    let index = 0;
                    let configObject = objectConfigPage.configObject;
                    if (configObject === undefined) {
                        return;
                    }
                    objectConfigPage.specialEntries.forEach((specialEntry) => {
                        const key = specialEntry.key;
                        const cond = specialEntry.cond;
                        if ((typeof cond !== "function") || cond(objectConfigPage)) {
                            const init = specialEntry.init;
                            if (typeof init === "function") {
                                configObject[key] = init();
                            }
                            const value = configObject[key];
                            if ((value !== undefined && value !== null) || !specialEntry.optional) {
                                indexByKey[key] = index;
                                const row = objectConfigPage.makeConfigRowForSpecialEntry(specialEntry, value, index);
                                if (row) {
                                    listModel.append(row);
                                    ++index;
                                }
                                return;
                            }
                        }
                        indexByKey[key] = -1;
                    });
                    if (objectConfigPage.specialEntriesOnly) {
                        return;
                    }
                    Object.entries(configObject).forEach((configEntry) => {
                        const key = configEntry[0];
                        if (indexByKey[key] === undefined) {
                            indexByKey[key] = index;
                            listModel.append(objectConfigPage.makeConfigRow(configEntry, index++));
                        }
                    });
                }
            }
            delegate: ObjectConfigDelegate {
                objectConfigPage: objectConfigPage
            }
        }
    }
    CustomDialog {
        id: newValueDialog
        title: qsTr("Add new value")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: GridLayout {
            columns: 2
            Label {
                text: Array.isArray(objectConfigPage.configObject) ? qsTr("Index") : qsTr("Key")
            }
            TextEdit {
                id: keyTextEdit
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Type")
            }
            ComboBox {
                id: typeComboBox
                model: ["String", "Number", "Boolean", "Object", "Array"]
            }
        }
        onAccepted: objectConfigPage.addObject(newValueDialog.typedKey, newValueDialog.typedValue)
        property alias key: keyTextEdit.text
        readonly property var typedKey: Array.isArray(objectConfigPage.configObject) ? Number.parseInt(newValueDialog.key) : newValueDialog.key
        readonly property var typedValue: ["", 0, false, {}, []][typeComboBox.currentIndex]
    }

    property alias model: objectListView.model
    property bool specialEntriesOnly: false
    property var indexByKey: ({})
    property var specialEntriesByKey: ({
        "remoteIgnoredDevices.*": [
            {key: "deviceID", label: qsTr("Device ID"), type: "deviceid", desc: qsTr("The ID of the device to be ignored.")},
            {key: "name", label: qsTr("Device Name"), desc: qsTr("The name of the device being ignored (for informative purposes).")},
            {key: "address", label: qsTr("Address"), desc: qsTr("The address of the device being ignored (for informative purposes).")},
            {key: "time", label: qsTr("Time"), init: () => new Date().toISOString(), desc: qsTr("The time when this entry was added (for informative purposes).")},
        ]
    })
    property var specialEntries: []
    property var configObject: undefined
    property var parentObject: undefined
    property var childObjectTemplate: configTemplates[path]
    property bool canAdd: Array.isArray(configObject) && childObjectTemplate !== undefined
    property bool isDangerous: false
    property bool isLoading: false
    property bool hasUnsavedChanges: false
    property bool readOnly: false
    property string itemLabel
    property string path: ""
    property string configCategory
    property string helpUrl
    property var configTemplates: ({
        "devices": {deviceID: "", introducedBy: "", encryptionPassword: ""},
        "addresses": "dynamic",
        "defaults.device.addresses": "dynamic",
        "allowedNetworks": "",
        "defaults.device.allowedNetworks": "",
        "options.listenAddresses": "default",
        "options.globalAnnounceServers": "default",
        "options.alwaysLocalNet": "",
        "ignoredFolders": {id: "", label: "", time: ""},
        "remoteIgnoredDevices": {deviceID: "", name: "", time: "", address: ""},
    })
    property list<var> labelKeys: ["label", "name", "id", "deviceID"]
    readonly property int standardButtons: (configCategory.length > 0) ? (Dialog.Ok | Dialog.Cancel | Dialog.Help) : (Dialog.Ok | Dialog.Cancel)
    required property StackView stackView
    property Page parentPage
    property Label objectNameLabel
    property list<Action> actions: [
        Action {
            text: qsTr("Help")
            enabled: objectConfigPage.helpUrl.length > 0
            icon.source: App.faUrlBase + "question"
            onTriggered: Qt.openUrlExternally(objectConfigPage.helpUrl)
        },
        Action {
            text: qsTr("Add")
            enabled: objectConfigPage.canAdd
            icon.source: App.faUrlBase + "plus"
            onTriggered: objectConfigPage.showNewValueDialog()
        }
    ]
    property list<Action> extraActions: []

    function makeConfigRow(configEntry, index, canAdd, childObjectTemplate) {
        const key = configEntry[0];
        const value = configEntry[1];
        const isArray = Array.isArray(objectConfigPage.configObject);
        const row = {key: key, value: value, type: typeof value, index: index, isArray: isArray, desc: ""};
        isArray ? computeArrayElementLabel(row) : (row.label = uncamel(key));
        handleReadOnlyMode(row);
        return row;
    }

    function computeArrayElementLabel(row) {
        for (const nestedKey of objectConfigPage.labelKeys) {
            const nestedValue = row.value[nestedKey];
            const nestedType = typeof nestedValue;
            if ((nestedType === "string" && nestedValue.length > 0) || nestedType === "number") {
                row.label = nestedValue;
                return;
            }
        }
        row.label = itemLabel.length ? itemLabel : uncamel(typeof row.value);
    }

    function makeConfigRowForSpecialEntry(specialEntry, value, index) {
        if ((specialEntry.value = value ?? specialEntry.defaultValue) === undefined) {
            return undefined;
        }
        specialEntry.index = index;
        specialEntry.isArray = Array.isArray(objectConfigPage.configObject);
        specialEntry.type = specialEntry.type ?? typeof specialEntry.value;
        specialEntry.label = specialEntry.label ?? uncamel(specialEntry.isArray ? specialEntry.key : specialEntry.type);
        specialEntry.desc = specialEntry.desc ?? "";
        handleReadOnlyMode(specialEntry);
        return specialEntry;
    }

    function handleReadOnlyMode(entry) {
        if (objectConfigPage.readOnly && entry.type !== "object") {
            entry.type = "readonly";
        }
    }

    function updateValue(index, key, value) {
        const configObject = objectConfigPage.configObject;
        const currentValue = configObject[key];
        if (currentValue === value) {
            return;
        }

        // update config object and list model, flag unsaved changes
        configObject[key] = value;
        objectConfigPage.hasUnsavedChanges = true;
        listModel.setProperty(index, "value", value);

        // update object name label (in parent page) and title for array elements
        const parentPage = objectConfigPage.parentPage;
        if (Array.isArray(parentPage?.configObject)) {
            const parentRow = parentPage.model.get(objectConfigPage.objectNameLabel.modelIndex);
            const parentValue = parentRow.value ?? {};
            parentValue[key] = value;
            parentRow.value = parentValue;
            parentPage.computeArrayElementLabel(parentRow);
            objectConfigPage.objectNameLabel.text = objectConfigPage.title = parentRow.label;
        }

        // update "ignorePerms" when setting "path" to a place where permissions should be ignored
        if (key === "path" && configObject.ignorePerms === false && App.shouldIgnorePermissions(value)) {
            const ignorePermsIndex = objectConfigPage.indexByKey.ignorePerms;
            if (ignorePermsIndex >= 0) {
                objectConfigPage.updateValue(ignorePermsIndex, "ignorePerms", true);
            }
        }
    }

    function swapObjects(modelData, moveDelta) {
        if (!modelData.isArray) {
            return;
        }
        const index = modelData.index;
        const swapIndex = index + moveDelta;
        const length = objectConfigPage.configObject.length;
        if (index >= 0 && index < length && swapIndex >= 0 && swapIndex < length) {
            const obj = objectConfigPage.configObject[index];
            const swapObj = objectConfigPage.configObject[swapIndex];
            objectConfigPage.configObject[swapIndex] = obj;
            objectConfigPage.configObject[index] = swapObj;
            objectConfigPage.hasUnsavedChanges = true;
            listModel.move(index, swapIndex, 1);
            listModel.set(index, {index: index, key: index});
            listModel.set(swapIndex, {index: swapIndex, key: swapIndex});
        }
    }

    function removeObjects(modelData, count) {
        if (!modelData.isArray) {
            return;
        }
        const index = modelData.index;
        const length = objectConfigPage.configObject.length;
        if (index >= 0 && index < length) {
            objectConfigPage.configObject.splice(index, count);
            listModel.remove(index, count);
        }
        for (let i = index, end = listModel.count; i !== end; ++i) {
            listModel.set(i, {index: i, key: i});
        }
        objectConfigPage.hasUnsavedChanges = true;
    }

    function addObject(key, object) {
        if (Array.isArray(objectConfigPage.configObject)) {
            const length = objectConfigPage.configObject.length;
            if (typeof key === "number" && key >= 0 && key <= length) {
                objectConfigPage.configObject.splice(key, 0, object);
                listModel.insert(key, objectConfigPage.makeConfigRow([key.toString(), object], key));
                for (let i = key + 1, end = listModel.count; i !== end; ++i) {
                    listModel.set(i, {index: i, key: i});
                }
            } else {
                App.showError(qsTr("Unable to add %1 because specified index is invalid.").arg(typeof object));
                return;
            }
        } else {
            if (typeof key === "string" && key !== "" && objectConfigPage.configObject[key] === undefined) {
                objectConfigPage.configObject[key] = object;
                listModel.append(objectConfigPage.makeConfigRow([key, object], listModel.count))
            } else {
                App.showError(qsTr("Unable to add %1 because specified key is invalid.").arg(typeof object));
                return;
            }
        }
        objectConfigPage.hasUnsavedChanges = true;
    }

    function disableInitialProperties() {
        // prevent changing settings that should only be set initially, e.g. folder/device ID
        [0, 3].forEach((i) => listModel.get(i).enabled === true && listModel.setProperty(i, "enabled", false));
    }

    function showNewValueDialog(key) {
        const template = objectConfigPage.childObjectTemplate;
        if (template !== undefined) {
            const newValue = typeof template === "object" ? Object.assign({}, template) : template;
            objectConfigPage.addObject(key ?? objectConfigPage.configObject.length, newValue);
        } else {
            newValueDialog.key = key ?? (Array.isArray(objectConfigPage.configObject) ? objectConfigPage.configObject.length : "");
            newValueDialog.visible = true;
        }
    }

    function uncamel(input) {
        const reservedStrings = [
            'IDs', 'ID', // substrings must come AFTER longer keywords containing them
            'URL', 'UR',
            'API', 'QUIC', 'TCP', 'UDP', 'NAT', 'LAN', 'WAN',
            'KiB', 'MiB', 'GiB', 'TiB'
        ];
        if (!input || typeof input !== 'string') return '';
        const placeholders = {};
        let counter = 0;
        reservedStrings.forEach(word => {
            const placeholder = `__RSV${counter}__`;
            const re = new RegExp(word, 'g');
            input = input.replace(re, placeholder);
            placeholders[placeholder] = word;
            counter++;
        });
        input = input.replace(/([a-z0-9])([A-Z])/g, '$1 $2');
        Object.entries(placeholders).forEach(([ph, word]) => {
            input = input.replace(new RegExp(ph, 'g'), ` ${word} `);
        });
        let parts = input.split(' ');
        const lastPart = parts.pop();
        switch (lastPart) {
            case 'S': parts.push('(seconds)'); break;
            case 'M': parts.push('(minutes)'); break;
            case 'H': parts.push('(hours)'); break;
            case 'Ms': parts.push('(milliseconds)'); break;
            default: parts.push(lastPart); break;
        }
        parts = parts.map(part => {
            const match = reservedStrings.find(w => w.toUpperCase() === part.toUpperCase());
            return match || part.charAt(0).toUpperCase() + part.slice(1);
        });
        return parts.join(' ').replace(/\s+/g, ' ').trim();
    }
}
