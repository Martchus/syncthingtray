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
        model: {
            const folders = app.connection.rawConfig?.folders;
            if (Array.isArray(folders)) {
                const folder = folders.find((folder) => folder.id === dirId)
                if (folder) {
                    const folderSettings = Object.entries(folder).sort().map((folderArray) => {
                                                                                 return {key: folderArray[0], value: folderArray[1],
                                                                                     type: typeof folderArray[1], label: uncamel(folderArray[0])};
                                                                             });
                    return folderSettings;
                }
            }
            return [];
        }
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
                                Layout.fillWidth: true
                                text: modelData.value
                                font.weight: Font.Light
                            }
                        }
                        RoundButton {
                            hoverEnabled: true
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            ToolTip.visible: hovered || pressed
                            ToolTip.text: qsTr("Open help")
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "question"
                            icon.width: 12
                            icon.height: 12
                            onClicked: Qt.openUrlExternally("https://docs.syncthing.net/users/config#config-option-folder." + modelData.key.toLowerCase())
                        }
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
                                Layout.fillWidth: true
                                text: modelData.value
                                font.weight: Font.Light
                            }
                        }
                        RoundButton {
                            hoverEnabled: true
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            ToolTip.visible: hovered || pressed
                            ToolTip.text: qsTr("Open help")
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "question"
                            icon.width: 12
                            icon.height: 12
                            onClicked: Qt.openUrlExternally("https://docs.syncthing.net/users/config#config-option-folder." + modelData.key.toLowerCase())
                        }
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
                        }
                        RoundButton {
                            hoverEnabled: true
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            ToolTip.visible: hovered || pressed
                            ToolTip.text: qsTr("Open help")
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "question"
                            icon.width: 12
                            icon.height: 12
                            onClicked: Qt.openUrlExternally("https://docs.syncthing.net/users/config#config-option-folder." + modelData.key.toLowerCase())
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
                        RoundButton {
                            hoverEnabled: true
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            ToolTip.visible: hovered || pressed
                            ToolTip.text: qsTr("Open help")
                            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            icon.source: app.faUrlBase + "question"
                            icon.width: 12
                            icon.height: 12
                            onClicked: Qt.openUrlExternally("https://docs.syncthing.net/users/config#config-option-folder." + modelData.key.toLowerCase())
                        }
                        Switch {
                            id: booleanSwitch
                            checked: modelData.value
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
