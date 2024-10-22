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
                        contentItem: RowLayout {
                            TextField {
                                id: editedStringValue
                                Layout.fillWidth: true
                                text: modelData.value
                                enabled: modelData?.enabled ?? true
                                onAccepted: stringDlg.accept()
                            }
                            CopyPasteButtons {
                                edit: editedStringValue
                            }
                        }
                        onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, editedStringValue.text)
                        onRejected: editedStringValue.text = objectConfigPage.configObject[modelData.key]
                        onHelpRequested: helpButton.clicked()
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "options"
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
                                Layout.fillWidth: true
                                text: optionsValue.editText // `${optionsValue.editText} (${modelData.value})`
                                elide: Text.ElideRight
                                font.weight: Font.Light
                            }
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        HelpButton {
                            id: optionsHelpButton
                            configCategory: objectConfigPage.configCategory
                        }
                    }
                    onClicked: optionsDlg.visible = true
                    Dialog {
                        id: optionsDlg
                        anchors.centerIn: Overlay.overlay
                        title: modelData.label
                        standardButtons: objectConfigPage.standardButtons
                        modal: true
                        width: parent.width - 20
                        contentItem: ColumnLayout {
                            id: optionsDlgLayout
                            Layout.fillWidth: true
                            ComboBox {
                                id: optionsValue
                                Layout.fillWidth: true
                                model: modelData.options
                                editable: true
                                valueRole: "value"
                                textRole: "label"
                                enabled: modelData?.enabled ?? true
                                onAccepted: optionsDlg.accept()
                                Component.onCompleted: editText = textForValue(modelData.value)
                                readonly property var currentOption: modelData.options.get(optionsValue.currentIndex)
                                readonly property string currentValueOrEditText: {
                                    const currentOption = optionsValue.currentOption;
                                    return (currentOption?.label === optionsValue.editText) ? currentOption?.value : optionsValue.editText;
                                }
                                function textForValue(value) {
                                    const displayText = optionsValue.textAt(optionsValue.indexOfValue(value));
                                    return displayText.length === 0 ? value : displayText;
                                }
                            }
                            ScrollView {
                                id: optionsHelpFlickable
                                Layout.fillWidth: true
                                // FIXME: scrolling does not work
                                //ScrollBar.vertical.policy: ScrollBar.AlwaysOn
                                contentWidth: optionsHelpLabel.contentWidth
                                contentHeight: optionsHelpLabel.contentHeight
                                Label {
                                    id: optionsHelpLabel
                                    wrapMode: Text.WordWrap
                                    visible: text.length > 0
                                    width: optionsHelpFlickable.width
                                    clip: false
                                    text: {
                                        const currentOption = optionsValue.currentOption;
                                        return (currentOption?.label === optionsValue.editText) ? (currentOption?.desc ?? "") : qsTr("A custom value has been entered.");
                                    }
                                }
                            }

                        }
                        onAccepted: {
                            const changeHandler = modelData.onChanged;
                            objectConfigPage.updateValue(modelData.index, modelData.key, optionsValue.currentValueOrEditText);
                            if (typeof changeHandler === "function") {
                                changeHandler(objectConfigPage.configObject);
                            }
                        }
                        onRejected: optionsValue.editText = optionsValue.textForValue(objectConfigPage.configObject[modelData.key])
                        onHelpRequested: optionsHelpButton.clicked()
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "devices"
                ItemDelegate {
                    id: devicesDelegate
                    width: parent?.width
                    contentItem: ColumnLayout {
                        width: objectListView.width
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                            }
                            ArrayElementButtons {
                                page: objectConfigPage
                                rowData: modelData
                            }
                            HelpButton {
                                id: devicesHelpButton
                                configCategory: objectConfigPage.configCategory
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Repeater {
                                model: devicesDelegate.devicesModel
                                ItemDelegate {
                                    id: deviceEntry
                                    Layout.fillWidth: true
                                    contentItem: RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            id: deviceNameOrIdLabel
                                            Layout.fillWidth: true
                                            text: app.connection.deviceNameOrId(modelData.deviceID)
                                            elide: Text.ElideRight
                                            font.weight: Font.Light
                                        }
                                        Switch {
                                            id: deviceSwitch
                                            checked: devicesDelegate.isDeviceEnabled(modelData.deviceID)
                                            onCheckedChanged: devicesDelegate.setDeviceEnabled(modelData.deviceID, deviceSwitch.checked)
                                        }
                                        RoundButton {
                                            id: encryptionButton
                                            display: AbstractButton.IconOnly
                                            text: deviceEntry.isEncryptionEnabled ? qsTr("Change encryption password") : qsTr("Set encryption password")
                                            enabled: deviceSwitch.checked
                                            hoverEnabled: true
                                            Layout.preferredWidth: 36
                                            Layout.preferredHeight: 36
                                            ToolTip.visible: hovered || pressed
                                            ToolTip.text: text
                                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                                            icon.source: app.faUrlBase + (deviceEntry.isEncryptionEnabled ? "lock" : "unlock")
                                            icon.width: 20
                                            icon.height: 20
                                            onClicked: encryptionPasswordDlg.open()
                                        }
                                        Dialog {
                                            id: encryptionPasswordDlg
                                            anchors.centerIn: Overlay.overlay
                                            parent: Overlay.overlay
                                            title: qsTr("Set encryption password for sharing with \"%1\"").arg(deviceNameOrIdLabel.text)
                                            standardButtons: objectConfigPage.standardButtons
                                            modal: true
                                            width: parent.width - 20
                                            contentItem: RowLayout {
                                                TextField {
                                                    id: encryptionKeyValue
                                                    Layout.fillWidth: true
                                                    text: modelData.encryptionPassword
                                                    placeholderText: qsTr("If untrusted, enter encryption password")
                                                    onAccepted: encryptionKeyDlg.accept()
                                                }
                                                CopyPasteButtons {
                                                    edit: encryptionKeyValue
                                                }
                                            }
                                            onAccepted: {
                                                deviceEntry.isEncryptionEnabled = encryptionKeyValue.text.length > 0;
                                                devicesDelegate.setDeviceProperty(modelData.deviceID, "encryptionPassword", encryptionKeyValue.text);
                                            }
                                            onRejected: encryptionKeyValue.text = Qt.binding(() => modelData.encryptionPassword)
                                            onHelpRequested: Qt.openUrlExternally("https://docs.syncthing.net/users/untrusted.html")
                                        }
                                    }
                                    onClicked: deviceSwitch.toggle()
                                    required property var modelData
                                    property bool isEncryptionEnabled: modelData.encryptionPassword?.length > 0
                                }
                            }
                        }
                    }
                    required property var modelData
                    property var devicesModel: {
                        const devices = [];
                        const sharedDevices = objectConfigPage.configObject[modelData.key];
                        const myId = app.connection.myId;
                        for (const sharedDev of sharedDevices) {
                            if (sharedDev.deviceID !== myId) {
                                devices.push(sharedDev);
                            }
                        }
                        const otherDevices = app.connection.deviceIds;
                        for (const otherDevID of otherDevices) {
                            if (otherDevID !== myId && devices.find((existingDev) => existingDev.deviceID === otherDevID) === undefined) {
                                devices.push({deviceID: otherDevID, encryptionPassword: "", introducedBy: ""});
                            }
                        }
                        return devices;
                    }
                    function findDevice(deviceID) {
                        return objectConfigPage.configObject[modelData.key].find((device) => device.deviceID === deviceID);
                    }
                    function isDeviceEnabled(deviceID) {
                        return findDevice(deviceID) !== undefined;
                    }
                    function setDeviceEnabled(deviceID, enabled) {
                        const devices = objectConfigPage.configObject[modelData.key];
                        const index = devices.findIndex((device) => device.deviceID === deviceID);
                        if (enabled && index < 0) {
                            devices.push({deviceID: deviceID, encryptionPassword: "", introducedBy: ""});
                        } else if (!enabled && index > 0) {
                            devices.splice(index, 1);
                        }
                    }
                    function setDeviceProperty(deviceID, key, value) {
                        const device = findDevice(deviceID);
                        if (device !== undefined) {
                            device[key] = value;
                        }
                    }
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
                            onAccepted: numberDlg.accept()
                        }
                        onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, Number.fromLocaleString(Qt.locale(numberValidator.locale), editedNumberValue.text))
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
                            readonly property string labelKey: modelData.labelKey ?? ""
                        }
                        ArrayElementButtons {
                            page: objectConfigPage
                            rowData: modelData
                        }
                        HelpButton {
                            configCategory: objectConfigPage.configCategory
                        }
                    }
                    onClicked: {
                        const currentPath = objectConfigPage.path;
                        const neestedPath = currentPath.length > 0 ? `${currentPath}.${modelData.key}` : modelData.key;
                        objectConfigPage.stackView.push("ObjectConfigPage.qml", {
                                                                   title: objNameLabel.text,
                                                                   configObject: objectConfigPage.configObject[modelData.key],
                                                                   parentObject: objectConfigPage.configObject,
                                                                   stackView: objectConfigPage.stackView,
                                                                   parentPage: objectConfigPage,
                                                                   objectNameLabel: objNameLabel,
                                                                   path: neestedPath,
                                                                   helpUrl: modelData.helpUrl ?? "",
                                                                   itemLabel: modelData.itemLabel ?? "",
                                                                   configTemplates: objectConfigPage.configTemplates,
                                                                   specialEntries: objectConfigPage.specialEntriesByKey[neestedPath] ?? [],
                                                                   specialEntriesByKey: objectConfigPage.specialEntriesByKey ?? {}},
                                                               StackView.PushTransition)
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "boolean"
                ItemDelegate {
                    width: objectListView.width
                    onClicked: booleanSwitch.toggle()
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
                                    Layout.fillWidth: true
                                    visible: text.length > 0
                                    elide: Text.ElideRight
                                    font.weight: Font.Light
                                    Component.onCompleted: text = modelData.statusText ?? ""
                                }
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
                        }
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "function"
                ItemDelegate {
                    width: objectListView.width
                    onClicked: modelData.value()
                    contentItem: ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: modelData.label
                            elide: Text.ElideRight
                            font.weight: Font.Medium
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            elide: Text.ElideRight
                            font.weight: Font.Light
                            Component.onCompleted: text = modelData.statusText ?? ""
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
                            onClicked: objectConfigPage.updateValue(modelData.index, modelData.key, "")
                        }
                        RoundButton {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            display: AbstractButton.IconOnly
                            text: qsTr("Edit manually")
                            ToolTip.text: text
                            ToolTip.visible: hovered || pressed
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "pencil"
                            icon.width: 20
                            icon.height: 20
                            onClicked: manualFileDlg.open()
                        }
                        HelpButton {
                            id: fileHelpButton
                            configCategory: objectConfigPage.configCategory
                        }
                    }
                    onClicked: fileDlg.open()
                    FileDialog {
                        id: fileDlg
                        title: modelData.label
                        onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, app.resolveUrl(fileDlg.selectedFile))
                    }
                    Dialog {
                        id: manualFileDlg
                        anchors.centerIn: Overlay.overlay
                        title: modelData.label
                        standardButtons: objectConfigPage.standardButtons
                        modal: true
                        width: parent.width - 20
                        contentItem: TextField {
                            id: editedFileValue
                            text: modelData.value
                            onAccepted: manualFileDlg.accept()
                        }
                        onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, editedFileValue.text)
                        onHelpRequested: fileHelpButton.clicked()
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
                            onClicked: objectConfigPage.updateValue(modelData.index, modelData.key, "")
                        }
                        RoundButton {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            display: AbstractButton.IconOnly
                            text: qsTr("Edit manually")
                            ToolTip.text: text
                            ToolTip.visible: hovered || pressed
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "pencil"
                            icon.width: 20
                            icon.height: 20
                            onClicked: manualFolderDlg.open()
                        }
                        HelpButton {
                            id: folderHelpButton
                            configCategory: objectConfigPage.configCategory
                        }
                    }
                    onClicked: folderDlg.open()
                    FolderDialog {
                        id: folderDlg
                        title: modelData.label
                        currentFolder: encodeURIComponent(folderpathValue.text)
                        onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, app.resolveUrl(folderDlg.selectedFolder))
                    }
                    Dialog {
                        id: manualFolderDlg
                        anchors.centerIn: Overlay.overlay
                        title: modelData.label
                        standardButtons: objectConfigPage.standardButtons
                        modal: true
                        width: parent.width - 20
                        contentItem: TextField {
                            id: editedFolderValue
                            text: modelData.value
                            onAccepted: manualFolderDlg.accept()
                        }
                        onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, editedFolderValue.text)
                        onHelpRequested: folderHelpButton.clicked()
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
    property bool specialEntriesOnly: false
    property var specialEntriesByKey: ({})
    property var specialEntries: []
    property var configObject: undefined
    property var parentObject: undefined
    property var childObjectTemplate: configTemplates[path]
    property bool canAdd: Array.isArray(configObject) && childObjectTemplate !== undefined
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
            icon.source: app.faUrlBase + "question"
            onTriggered: Qt.openUrlExternally(objectConfigPage.helpUrl)
        },
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
        return row;
    }

    function makeConfigRowForSpecialEntry(specialEntry, value, index) {
        specialEntry.index = index;
        specialEntry.isArray = Array.isArray(objectConfigPage.configObject);
        specialEntry.value = value ?? specialEntry.defaultValue;
        specialEntry.type = specialEntry.type ?? typeof specialEntry.value;
        specialEntry.label = specialEntry.label ?? uncamel(specialEntry.isArray ? specialEntry.key : specialEntry.type);
        specialEntry.desc = specialEntry.desc ?? "";
        return specialEntry;
    }

    function updateValue(index, key, value) {
        objectConfigPage.configObject[key] = value;
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

    function disableFirst() {
        // note: Mean to prevent editing folder/device ID initially saving new folder/device.
        if (listModel.get(0).enabled === true) {
            listModel.setProperty(0, "enabled", false);
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
