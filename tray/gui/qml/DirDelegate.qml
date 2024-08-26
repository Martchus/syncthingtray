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
                onTriggered: (source) => app.openDir(modelData.path)
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Browse remote files")
                onTriggered: (source) => mainView.stackView.push(fileView, {dirName: modelData.name, dirId: modelData.dirId}, StackView.PushTransition)
            }
        ]

        Component {
            id: fileView
            Page {
                title: qsTr("Remote/global tree of \"%1\"").arg(dirName)
                TreeView {
                    anchors.fill: parent
                    model: app.createFileModel(dirId)
                    selectionModel: ItemSelectionModel {}
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
                                ToolTip.visible: toolTipData.length > 0 && (hovered || pressed)
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
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
