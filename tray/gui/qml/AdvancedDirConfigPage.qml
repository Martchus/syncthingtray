import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.qmlmodels

Page {
    title: qsTr("Advanced config of folder \"%1\"").arg(dirName)
    actions: [
        Action {
            text: qsTr("Apply (not impleented yet)")
            icon.source: app.faUrlBase + "check"
            onTriggered: (source) => {
                         }
        }
    ]
    ListView {
        id: advancedConfigListView
        anchors.fill: parent
        property var folderConfig: {
            const folders = app.connection.rawConfig?.folders;
            return Array.isArray(folders) ? folders.find((folder) => folder.id === dirId) : {};
        }
        model:  Object.entries(folderConfig).sort().map((folderArray) => {
                                                            return {key: folderArray[0], value: folderArray[1],
                                                                type: typeof folderArray[1], label: uncamel(folderArray[0])};
                                                        })
        delegate: DelegateChooser {
            role: "type"
            DelegateChoice {
                roleValue: "string"
                ItemDelegate {
                    width: advancedConfigListView.width
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
                            key: modelData.key
                        }
                    }
                    onClicked: stringDlg.visible = true
                    Dialog {
                        id: stringDlg
                        title: modelData.label
                        standardButtons: Dialog.Ok | Dialog.Cancel | Dialog.Help
                        modal: true
                        width: parent.width - 20
                        contentItem: TextField {
                            id: editedStringValue
                            text: modelData.value
                            onAccepted: stringDlg.accpet()
                        }
                        onAccepted: {
                            advancedConfigListView.folderConfig[modelData.key] = editedStringValue.text
                            stringValue.text = editedStringValue.text
                        }
                        onRejected: editedStringValue.text = advancedConfigListView.folderConfig[modelData.key]
                        onHelpRequested: helpButton.clicked()
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "number"
                ItemDelegate {
                    width: advancedConfigListView.width
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
                            key: modelData.key
                        }
                    }
                    onClicked: numberDlg.visible = true
                    Dialog {
                        id: numberDlg
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
                            advancedConfigListView.folderConfig[modelData.key] = number
                            numberValue.text = number
                        }
                        onRejected: editedNumberValue.text = advancedConfigListView.folderConfig[modelData.key]
                        onHelpRequested: helpButton.clicked()
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "object"
                ItemDelegate {
                    width: advancedConfigListView.width
                    contentItem: RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                font.weight: Font.Medium
                            }
                            Label {
                                Layout.fillWidth: true
                                text: "Unable to show/edit object at this point"
                                font.weight: Font.Light
                            }
                        }
                        HelpButton {
                            key: modelData.key
                        }
                    }
                    required property var modelData
                }
            }
            DelegateChoice {
                roleValue: "boolean"
                ItemDelegate {
                    width: advancedConfigListView.width
                    onClicked: booleanSwitch.toggle()
                    contentItem: RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                font.weight: Font.Medium
                            }
                        }
                        Switch {
                            id: booleanSwitch
                            checked: modelData.value
                            onCheckedChanged: advancedConfigListView.folderConfig[modelData.key] = booleanSwitch.checked
                        }
                        HelpButton {
                            key: modelData.key
                        }
                    }
                    required property var modelData
                }
            }
        }
    }
    required property string dirName
    required property string dirId
    required property list<Action> actions

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
