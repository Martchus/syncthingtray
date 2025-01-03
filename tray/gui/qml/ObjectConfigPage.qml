import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import Main

Page {
    id: objectConfigPage
    CustomListView {
        id: objectListView
        anchors.fill: parent
        model: ListModel {
            id: listModel
            dynamicRoles: true
            Component.onCompleted: listModel.loadEntries()
            function loadEntries() {
                listModel.clear();

                let index = 0;
                let handledKeys = new Set();
                let configObject = objectConfigPage.configObject;
                if (configObject === undefined) {
                    return;
                }
                objectConfigPage.specialEntries.forEach((specialEntry) => {
                    const key = specialEntry.key;
                    const cond = specialEntry.cond;
                    if ((typeof cond !== "function") || cond(objectConfigPage)) {
                        listModel.append(objectConfigPage.makeConfigRowForSpecialEntry(specialEntry, configObject[key], index++));
                    }
                    handledKeys.add(key);
                });
                if (objectConfigPage.specialEntriesOnly) {
                    return;
                }
                Object.entries(configObject).forEach((configEntry) => {
                    if (!handledKeys.has(configEntry[0])) {
                        listModel.append(objectConfigPage.makeConfigRow(configEntry, index++));
                    }
                });
            }
        }
        footer: LoadingPane {
            visible: objectConfigPage.isLoading
            width: objectListView.width
        }
        delegate: ObjectConfigDelegate {
            objectConfigPage: objectConfigPage
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
    property var specialEntriesByKey: ({})
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
    property var configTemplates: ({})
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
        if (!isArray) {
            row.label = uncamel(key);
            row.labelKey = "";
        } else {
            const nestedKey = "deviceID";
            const nestedValue = value[nestedKey];
            const nestedType = typeof nestedValue;
            const hasNestedValue = nestedType === "string" || nestedType === "number";
            row.label = hasNestedValue ? nestedValue : (itemLabel.length ? itemLabel : uncamel(typeof value));
            row.labelKey = hasNestedValue ? nestedKey : "";
        }
        handleReadOnlyMode(row);
        return row;
    }

    function makeConfigRowForSpecialEntry(specialEntry, value, index) {
        specialEntry.index = index;
        specialEntry.isArray = Array.isArray(objectConfigPage.configObject);
        specialEntry.value = value ?? specialEntry.defaultValue;
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
        configObject[key] = value;
        objectConfigPage.hasUnsavedChanges = true;
        listModel.setProperty(index, "value", value);
        if (Array.isArray(objectConfigPage.parentPage?.configObject) && objectConfigPage.objectNameLabel?.labelKey === key) {
            objectConfigPage.title = value;
            objectConfigPage.objectNameLabel.text = value;
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
            const newValue = template === "object" ? Object.assign({}, template) : template;
            objectConfigPage.addObject(key ?? objectConfigPage.configObject.length, newValue);
        } else {
            newValueDialog.key = key ?? (Array.isArray(objectConfigPage.configObject) ? objectConfigPage.configObject.length : "");
            newValueDialog.visible = true;
        }
    }

    function uncamel(input) {
        input = input.replace(/(.)([A-Z][a-z]+)/g, '$1 $2').replace(/([a-z0-9])([A-Z])/g, '$1 $2');
        const parts = input.split(' ');
        const lastPart = parts.splice(-1)[0];
        switch (lastPart) {
        case "S":
            parts.push('(seconds)');
            break;
        case "M":
            parts.push('(minutes)');
            break;
        case "H":
            parts.push('(hours)');
            break;
        case "Ms":
            parts.push('(milliseconds)');
            break;
        default:
            parts.push(lastPart);
            break;
        }
        input = parts.join(' ');
        return input.charAt(0).toUpperCase() + input.slice(1);
    }
}
