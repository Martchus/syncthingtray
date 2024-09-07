import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.qmlmodels

Page {
    id: objectConfigPage
    ListView {
        id: objectListView
        anchors.fill: parent
        model: Object.entries(objectConfigPage.configObject).sort().map(objectConfigPage.makeConfigRow)
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
                                font.weight: Font.Medium
                            }
                            Label {
                                id: stringValue
                                Layout.fillWidth: true
                                text: modelData.value
                                font.weight: Font.Light
                            }
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
                        standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Help
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
                                font.weight: Font.Medium
                            }
                            Label {
                                id: numberValue
                                Layout.fillWidth: true
                                text: modelData.value
                                font.weight: Font.Light
                            }
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
                        standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Help
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
                        Image {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            visible: modelData.isArray
                            source: modelData.isArray ? (app.faUrlBase + "hashtag") : ("")
                            width: 16
                            height: 16
                        }
                        Label {
                            id: objNameLabel
                            Layout.fillWidth: true
                            text: modelData.label
                            font.weight: Font.Medium
                            readonly property string key: modelData.key
                            readonly property string labelKey: modelData.labelKey
                        }
                        HelpButton {
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    onClicked: objectConfigPage.stackView.push("ObjectConfigPage.qml", {title: modelData.label, configObject: objectConfigPage.configObject[modelData.key], stackView: objectConfigPage.stackView, parentPage: objectConfigPage, objectNameLabel: objNameLabel}, StackView.PushTransition)
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
                            font.weight: Font.Medium
                        }
                        Switch {
                            id: booleanSwitch
                            checked: modelData.value
                            onCheckedChanged: objectConfigPage.configObject[modelData.key] = booleanSwitch.checked
                        }
                        HelpButton {
                            configCategory: objectConfigPage.configCategory
                            key: modelData.key
                        }
                    }
                    required property var modelData
                }
            }
        }
    }
    property alias model: objectListView.model
    required property var configObject
    property string configCategory
    required property StackView stackView
    property Page parentPage
    property Label objectNameLabel
    property list<Action> actions

    function makeConfigRow(configEntry, index) {
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
            row.label = hasNestedValue ? `${key}: ${nestedValue}` : key;
            row.labelKey = hasNestedValue ? nestedKey : "";
        }
        return row;
    }

    function updateValue(key, value) {
        objectConfigPage.configObject[key] = value
        if (Array.isArray(objectConfigPage.parentPage?.configObject) && objectConfigPage.objectNameLabel?.labelKey === key) {
            const label = `${objectConfigPage.objectNameLabel.key}: ${value}`
            objectConfigPage.title = label
            objectConfigPage.objectNameLabel.text = label
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
