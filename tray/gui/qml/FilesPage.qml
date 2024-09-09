import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

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
                acceptedButtons: Qt.LeftButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onTapped: treeView.toggleExpanded(row)
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onTapped: contextMenu.open()
            }
            TapHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onLongPressed: treeView.model.triggerAction("toggle-selection-single", treeView.index(row, 0))
            }
            TapHandler {
                acceptedDevices: PointerDevice.TouchScreen
                onTapped: treeView.toggleExpanded(row)
                onLongPressed: contextMenu.open()
            }
            Menu {
                id: contextMenu
                MenuItem {
                    text: qsTr("Refresh")
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    onClicked: treeView.model.fetchMore(treeView.index(row, 0))
                }
                MenuItem {
                    text: checkStateData !== Qt.Checked ? qsTr("Select") : qsTr("Deselect")
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    onClicked: treeView.model.triggerAction("toggle-selection-single", treeView.index(row, 0))
                }
            }
        }
    }

    required property string dirName
    required property string dirId
}
