import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.qmlmodels

import Main

DelegateChooser {
    role: "type"
    DelegateChoice {
        roleValue: "readonly"
        ItemDelegate {
            width: objectListView.width
            contentItem: RowLayout {
                ColumnLayout {
                    Layout.fillWidth: true
                    enabled: modelData.enabled ?? true
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
                IconOnlyButton {
                    text: qsTr("Copy")
                    icon.source: App.faUrlBase + "files-o"
                    onClicked: App.copyText(modelData.value)
                }
                HelpButton {
                    configCategory: objectConfigPage.configCategory
                }
            }
            required property var modelData
        }
    }
    DelegateChoice {
        roleValue: "string"
        ItemDelegate {
            width: objectListView.width
            contentItem: RowLayout {
                ColumnLayout {
                    Layout.fillWidth: true
                    enabled: modelData.enabled ?? true
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
                popupType: App.nativePopups ? Popup.Native : Popup.Item
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
                popupType: App.nativePopups ? Popup.Native : Popup.Item
                anchors.centerIn: Overlay.overlay
                title: modelData.label
                standardButtons: objectConfigPage.standardButtons
                modal: true
                width: parent.width - 20
                contentItem: ScrollView {
                    id: optionsScrollView
                    contentWidth: availableWidth
                    ColumnLayout {
                        id: optionsDlgLayout
                        width: optionsScrollView.width - optionsScrollView.effectiveScrollBarWidth
                        ComboBox {
                            id: optionsValue
                            Layout.fillWidth: true
                            model: modelData.options
                            editable: true
                            valueRole: "value"
                            textRole: "label"
                            enabled: modelData?.enabled ?? true
                            onAccepted: optionsDlg.accept()
                            Component.onCompleted: {
                                //popup.popupType = Popup.Native;
                                editText = textForValue(modelData.value);
                            }
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
                        Label {
                            id: optionsHelpLabel
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            visible: text.length > 0
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
                                    text: App.connection.deviceNameOrId(modelData.deviceID)
                                    elide: Text.ElideRight
                                    font.weight: Font.Light
                                }
                                Switch {
                                    id: deviceSwitch
                                    checked: devicesDelegate.isDeviceEnabled(modelData.deviceID)
                                    onCheckedChanged: devicesDelegate.setDeviceEnabled(modelData.deviceID, deviceSwitch.checked)
                                }
                                IconOnlyButton {
                                    id: encryptionButton
                                    text: deviceEntry.isEncryptionEnabled ? qsTr("Change encryption password") : qsTr("Set encryption password")
                                    enabled: deviceSwitch.checked
                                    icon.source: App.faUrlBase + (deviceEntry.isEncryptionEnabled ? "lock" : "unlock")
                                    onClicked: encryptionPasswordDlg.open()
                                }
                                Dialog {
                                    id: encryptionPasswordDlg
                                    popupType: App.nativePopups ? Popup.Native : Popup.Item
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
                const myId = App.connection.myId;
                for (const sharedDev of sharedDevices) {
                    if (sharedDev.deviceID !== myId) {
                        devices.push(sharedDev);
                    }
                }
                const otherDevices = App.connection.deviceIds;
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
                    objectConfigPage.hasUnsavedChanges = true;
                } else if (!enabled && index > 0) {
                    devices.splice(index, 1);
                    objectConfigPage.hasUnsavedChanges = true;
                }
            }
            function setDeviceProperty(deviceID, key, value) {
                const device = findDevice(deviceID);
                if (device !== undefined) {
                    device[key] = value;
                    objectConfigPage.hasUnsavedChanges = true;
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
                popupType: App.nativePopups ? Popup.Native : Popup.Item
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
                                                    isDangerous: objectConfigPage.isDangerous,
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
                        wrapMode: Text.WordWrap
                        Component.onCompleted: text = modelData.statusText ?? ""
                    }
                }
                Switch {
                    id: booleanSwitch
                    checked: modelData.value
                    onCheckedChanged: {
                        objectConfigPage.configObject[modelData.key] = booleanSwitch.checked;
                        objectConfigPage.hasUnsavedChanges = true;
                    }
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
                    enabled: modelData.enabled ?? true
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
                IconOnlyButton {
                    text: qsTr("Clear")
                    enabled: modelData.enabled ?? true
                    icon.source: App.faUrlBase + "undo"
                    onClicked: objectConfigPage.updateValue(modelData.index, modelData.key, "")
                }
                IconOnlyButton {
                    text: qsTr("Edit manually")
                    enabled: modelData.enabled ?? true
                    icon.source: App.faUrlBase + "pencil"
                    onClicked: manualFileDlg.open()
                }
                HelpButton {
                    id: fileHelpButton
                    configCategory: objectConfigPage.configCategory
                }
            }
            onClicked: (modelData.enabled ?? true) && fileDlg.open()
            FileDialog {
                id: fileDlg
                title: modelData.label
                onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, App.resolveUrl(fileDlg.selectedFile))
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
                    enabled: modelData.enabled ?? true
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
                IconOnlyButton {
                    text: qsTr("Clear")
                    enabled: modelData.enabled ?? true
                    icon.source: App.faUrlBase + "undo"
                    onClicked: objectConfigPage.updateValue(modelData.index, modelData.key, "")
                }
                IconOnlyButton {
                    text: qsTr("Edit manually")
                    enabled: modelData.enabled ?? true
                    icon.source: App.faUrlBase + "pencil"
                    onClicked: manualFolderDlg.open()
                }
                HelpButton {
                    id: folderHelpButton
                    configCategory: objectConfigPage.configCategory
                }
            }
            onClicked: (modelData.enabled ?? true) && folderDlg.open()
            FolderDialog {
                id: folderDlg
                title: modelData.label
                currentFolder: encodeURIComponent(folderpathValue.text)
                onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, App.resolveUrl(folderDlg.selectedFolder))
            }
            Dialog {
                id: manualFolderDlg
                popupType: App.nativePopups ? Popup.Native : Popup.Item
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
    required property var objectConfigPage
}
