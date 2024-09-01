import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.qmlmodels
import Main

ExpandableDelegate {
    id: mainDelegateModel
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
        actions: [
            Action {
                text: qsTr("Rescan")
                enabled: !modelData.paused
                icon.source: app.faUrlBase + "refresh"
                onTriggered: (source) => app.connection.rescan(modelData.dirId)
            },
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                icon.source: app.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: (source) => app.connection[modelData.paused ? "resumeDirectories" : "pauseDirectories"]([modelData.dirId])
            },
            Action {
                text: qsTr("Open in file browser")
                icon.source: app.faUrlBase + "folder"
                onTriggered: (source) => app.openPath(modelData.path)
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Edit ignore patterns")
                onTriggered: (source) => mainView.stackView.push(ignorePatternView, {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Browse remote files")
                enabled: !modelData.paused
                onTriggered: (source) => mainView.stackView.push(fileView, {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            },
            Action {
                text: qsTr("Advanced config")
                onTriggered: (source) => mainView.stackView.push(advancedConfigComponent, {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            }
        ]

        Component {
            id: ignorePatternView
            Page {
                title: qsTr("Ignore patterns of \"%1\"").arg(dirName)
                Component.onCompleted: app.loadIgnorePatterns(dirId, textArea)
                actions: [
                    Action {
                        text: qsTr("Save")
                        icon.source: app.faUrlBase + "floppy-o"
                        onTriggered: (source) => app.saveIgnorePatterns(dirId, textArea)
                    }
                ]
                ScrollView {
                    anchors.fill: parent
                    TextArea {
                        id: textArea
                        anchors.fill: parent
                        enabled: false
                    }
                }
                BusyIndicator {
                    anchors.centerIn: parent
                    running: !textArea.enabled
                }

                required property string dirName
                required property string dirId
                required property list<Action> actions
            }
        }

        Component {
            id: advancedConfigComponent

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
        }

        Component {
            id: fileView
            Page {
                title: qsTr("Remote/global tree of \"%1\"").arg(dirName)
                TreeView {
                    id: treeView
                    anchors.fill: parent
                    model: app.createFileModel(dirId, treeView)
                    selectionModel: ItemSelectionModel {}
                    onExpanded: (row, depth) => {
                        const index = treeView.index(row, 0);
                        const model = treeView.model;
                        return model.canFetchMore(index) && model.fetchMore(index);
                    }
                    delegate: TreeViewDelegate {
                        indentation: 16
                        indicator: Item {
                            x: leftMargin + (depth * indentation)
                        }
                        contentItem: RowLayout {
                            width: parent.width
                            CheckBox {
                                visible: treeView.model.selectionModeEnabled && column === 0
                                checkState: checkStateData ?? Qt.Unchecked
                                onClicked: treeView.model.triggerAction("toggle-selection-single", treeView.index(row, 0))
                            }
                            Icon {
                                source: decorationData
                                width: 32
                                height: 32
                            }
                            Label {
                                Layout.fillWidth: true
                                text: textData ?? ''
                                ToolTip.text: toolTipData ?? ''
                                ToolTip.visible: (toolTipData !== undefined && toolTipData.length > 0) && (hovered || pressed)
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                                ToolTip.toolTip.onAboutToShow: ToolTip.text = toolTipData ?? ''
                            }
                        }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: contextMenu.open()
                        }
                        TapHandler {
                            onLongPressed: treeView.model.triggerAction("toggle-selection-single", treeView.index(row, 0))
                        }
                        Menu {
                            id: contextMenu
                            MenuItem {
                                text: qsTr("Refresh")
                                onClicked: treeView.model.fetchMore(treeView.index(row, 0))
                            }
                            MenuItem {
                                text: checkStateData !== Qt.Checked ? qsTr("Select") : qsTr("Deselect")
                                onClicked: treeView.model.triggerAction("toggle-selection-single", treeView.index(row, 0))
                            }
                        }
                    }
                }

                required property string dirName
                required property string dirId
            }
        }
    }
}
