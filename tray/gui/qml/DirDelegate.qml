import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
                TextArea {
                    id: textArea
                    anchors.fill: parent
                    enabled: false
                }
                BusyIndicator {
                    anchors.centerIn: parent
                    running: !textArea.enabled
                }

                required property string dirName
                required property string dirId
                property list<Action> actions
            }
        }

        Component {
            id: fileView
            Page {
                title: qsTr("Remote/global tree of \"%1\"").arg(dirName)
                TreeView {
                    id: treeView
                    anchors.fill: parent
                    model: app.createFileModel(dirId)
                    selectionModel: ItemSelectionModel {}
                    onExpanded: (row, depth) => {
                        const index = treeView.index(row, 0)
                        const model = treeView.model
                        model.canFetchMore(index) && model.fetchMore(index)
                    }
                    delegate: TreeViewDelegate {
                        indentation: 16
                        indicator: Item {
                            x: leftMargin + (depth * indentation)
                        }
                        contentItem: RowLayout {
                            width: parent.width
                            Icon {
                                source: decorationData
                                width: 32
                                height: 32
                            }
                            Label {
                                Layout.fillWidth: true
                                text: textData ?? ''
                                ToolTip.text: toolTipData ?? ''
                                ToolTip.visible: toolTipData !== undefined && (hovered || pressed)
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                            }
                        }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: contextMenu.open()
                        }
                        Menu {
                            id: contextMenu
                            MenuItem {
                                text: qsTr("Refresh")
                                onClicked: treeView.model.fetchMore(treeView.index(row, 0))
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
