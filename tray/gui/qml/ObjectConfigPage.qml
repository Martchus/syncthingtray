import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.qmlmodels

Page {
    id: objectConfigPage
    ListView {
        id: objectListView
        anchors.fill: parent
        ScrollIndicator.vertical: ScrollIndicator { }
        model: ListModel {
            id: listModel
            dynamicRoles: true
            Component.onCompleted: {
                let index = 0;
                let handledKeys = new Set();
                let configObject = objectConfigPage.configObject;
                objectConfigPage.specialEntries.forEach((specialEntry) => {
                    const key = specialEntry.key;
                    listModel.append(objectConfigPage.makeConfigRowForSpecialEntry(specialEntry, configObject[key], index++));
                    handledKeys.add(key);
                });
                Object.entries(configObject).forEach((configEntry) => {
                    if (!handledKeys.has(configEntry[0])) {
                        listModel.append(objectConfigPage.makeConfigRow(configEntry, index++));
                    }
                });
            }
        }
        delegate: DelegateChooser {
            role: "type"
            DelegateChoice {
                roleValue: "string"
                ItemDelegate {
                    width: objectListView.width
                    contentItem: RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                            }
                            Label {
                                id: stringValue
                                Layout.fillWidth: true
                                text: modelData.value
                                elide: Text.ElideRight
                                font.weight: Font.Light
                            }
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        HelpButton {
                            id: helpButton
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    onClicked: stringDlg.visible = true
                    Dialog {
                        id: stringDlg
                        anchors.centerIn: Overlay.overlay
                        title: modelData.label
                        standardButtons: objectConfigPage.standardButtons
                        modal: true
                        width: parent.width - 20
                        contentItem: TextField {
                            id: editedStringValue
                            text: modelData.value
                            onAccepted: stringDlg.accpet()
                        }
                        onAccepted: objectConfigPage.updateValue(modelData.key, stringValue.text = editedStringValue.text)
                        onRejected: editedStringValue.text = objectConfigPage.configObject[modelData.key]
                        onHelpRequested: helpButton.clicked()
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "number"
                ItemDelegate {
                    width: objectListView.width
                    contentItem: RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                            }
                            Label {
                                id: numberValue
                                Layout.fillWidth: true
                                text: modelData.value
                                elide: Text.ElideRight
                                font.weight: Font.Light
                            }
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        HelpButton {
                            id: numberHelpButton
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    onClicked: numberDlg.visible = true
                    Dialog {
                        id: numberDlg
                        anchors.centerIn: Overlay.overlay
                        title: modelData.label
                        standardButtons: objectConfigPage.standardButtons
                        modal: true
                        width: parent.width - 20
                        contentItem: TextField {
                            id: editedNumberValue
                            text: modelData.value
                            validator: DoubleValidator {
                                id: numberValidator
                                locale: "en"
                            }
                            onAccepted: numberDlg.accpet()
                        }
                        onAccepted: {
                            const number = Number.fromLocaleString(Qt.locale(numberValidator.locale), editedNumberValue.text)
                            objectConfigPage.updateValue(modelData.key, number)
                            numberValue.text = number
                        }
                        onRejected: editedNumberValue.text = objectConfigPage.configObject[modelData.key]
                        onHelpRequested: numberHelpButton.clicked()
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "object"
                ItemDelegate {
                    width: objectListView.width
                    contentItem: RowLayout {
                        Label {
                            id: objNameLabel
                            Layout.fillWidth: true
                            text: modelData.label
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                            readonly property string key: modelData.key
                            readonly property string labelKey: modelData.labelKey
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        HelpButton {
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    onClicked: objectConfigPage.stackView.push("ObjectConfigPage.qml", {title: objNameLabel.text, configObject: objectConfigPage.configObject[modelData.key], stackView: objectConfigPage.stackView, parentPage: objectConfigPage, objectNameLabel: objNameLabel, path: `${objectConfigPage.path}.${modelData.key}`, configTemplates: objectConfigPage.configTemplates}, StackView.PushTransition)
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "boolean"
                ItemDelegate {
                    width: objectListView.width
                    onClicked: booleanSwitch.toggle()
                    contentItem: RowLayout {
                        Label {
                            Layout.fillWidth: true
                            text: modelData.label
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }
                        Switch {
                            id: booleanSwitch
                            checked: modelData.value
                            onCheckedChanged: objectConfigPage.configObject[modelData.key] = booleanSwitch.checked
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        HelpButton {
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "filepath"
                ItemDelegate {
                    width: objectListView.width
                    contentItem: RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                            }
                            Label {
                                id: filepathValue
                                Layout.fillWidth: true
                                text: modelData.value
                                elide: Text.ElideRight
                                font.weight: Font.Light
                            }
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        RoundButton {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            display: AbstractButton.IconOnly
                            text: qsTr("Clear")
                            ToolTip.text: text
                            ToolTip.visible: hovered || pressed
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "undo"
                            icon.width: 20
                            icon.height: 20
                            onClicked: objectConfigPage.updateValue(modelData.key, filepathValue.text = "")
                        }
                        HelpButton {
                            id: helpButton
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    onClicked: fileDlg.open()
                    FileDialog {
                        id: fileDlg
                        title: modelData.label
                        onAccepted: objectConfigPage.updateValue(modelData.key, filepathValue.text = app.resolveUrl(fileDlg.selectedFile))
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "folderpath"
                ItemDelegate {
                    width: objectListView.width
                    contentItem: RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                            }
                            Label {
                                id: folderpathValue
                                Layout.fillWidth: true
                                text: modelData.value
                                elide: Text.ElideRight
                                font.weight: Font.Light
                            }
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        RoundButton {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            display: AbstractButton.IconOnly
                            text: qsTr("Clear")
                            ToolTip.text: text
                            ToolTip.visible: hovered || pressed
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "undo"
                            icon.width: 20
                            icon.height: 20
                            onClicked: objectConfigPage.updateValue(modelData.key, folderpathValue.text = "")
                        }
                        HelpButton {
                            id: helpButton
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    onClicked: folderDlg.open()
                    FolderDialog {
                        id: folderDlg
                        title: modelData.label
                        onAccepted: objectConfigPage.updateValue(modelData.key, folderpathValue.text = app.resolveUrl(folderDlg.selectedFolder))
                    }
                    required property var modelData
                }
            }
        }
    }
    Dialog {
        id: newValueDialog
        anchors.centerIn: Overlay.overlay
        title: qsTr("Add new value")
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        width: parent.width - 20
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
    property var specialEntries: []
    required property var configObject
    property var childObjectTemplate: configTemplates[path]
    property bool canAdd: Array.isArray(configObject) && childObjectTemplate !== undefined
    property string path: ""
    property string configCategory
    property var configTemplates: ({})
    readonly property int standardButtons: (configCategory.length > 0) ? (Dialog.Ok | Dialog.Cancel | Dialog.Help) : (Dialog.Ok | Dialog.Cancel)
    required property StackView stackView
    property Page parentPage
    property Label objectNameLabel
    property list<Action> actions: [
        Action {
            text: qsTr("Add")
            enabled: objectConfigPage.canAdd
            icon.source: app.faUrlBase + "plus"
            onTriggered: objectConfigPage.showNewValueDialog()
        }
    ]
    property list<Action> extraActions: []

    function makeConfigRow(configEntry, index, canAdd, childObjectTemplate) {
        const key = configEntry[0];
        const value = configEntry[1];
        const isArray = Array.isArray(objectConfigPage.configObject);
        const row = {key: key, value: value, type: typeof value, index: index, isArray: isArray};
        if (!isArray) {
            row.label = uncamel(key);
            row.labelKey = "";
        } else {
            const nestedKey = "deviceID";
            const nestedValue = value[nestedKey];
            const nestedType = typeof nestedValue;
            const hasNestedValue = nestedType === "string" || nestedType === "number";
            row.label = hasNestedValue ? nestedValue : uncamel(typeof value);
            row.labelKey = hasNestedValue ? nestedKey : "";
        }
        return row;
    }

    function makeConfigRowForSpecialEntry(specialEntry, value, index) {
        specialEntry.index = index;
        specialEntry.isArray = Array.isArray(objectConfigPage.configObject);
        specialEntry.value = value;
        specialEntry.type = specialEntry.type ?? typeof value;
        specialEntry.label = specialEntry.label ?? uncamel(specialEntry.isArray ? specialEntry.key : typeof value);
        return specialEntry;
    }

    function updateValue(key, value) {
        objectConfigPage.configObject[key] = value
        if (Array.isArray(objectConfigPage.parentPage?.configObject) && objectConfigPage.objectNameLabel?.labelKey === key) {
            objectConfigPage.title = value
            objectConfigPage.objectNameLabel.text = value
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
                app.showError(qsTr("Unable to add %1 because specified index is invalid.").arg(typeof object));
            }
        } else {
            if (typeof key === "string" && key !== "" && objectConfigPage.configObject[key] === undefined) {
                objectConfigPage.configObject[key] = object;
                listModel.append(objectConfigPage.makeConfigRow([key, object], listModel.count))
            } else {
                app.showError(qsTr("Unable to add %1 because specified key is invalid.").arg(typeof object));
            }
        }
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
