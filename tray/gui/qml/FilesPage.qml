import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

Page {
    title: qsTr("Remote/global tree of \"%1\"").arg(dirName)

    DelegateModel {
        id: delegateModel
        model: app.createFileModel(dirId, listView)
        delegate: ItemDelegate {
            width: listView.width
            contentItem: ColumnLayout {
                RowLayout {
                    Icon {
                        source: decorationData
                    }
                    Label {
                        Layout.fillWidth: true
                        text: name
                    }
                    CheckBox {
                        visible: delegateModel.model.selectionModeEnabled
                        checkState: checkStateData
                        onClicked: toggleCurrentIndex()
                    }
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onTapped: {
                    const modelIndex = delegateModel.modelIndex(index);
                    if (delegateModel.model.hasChildren(modelIndex)) {
                        delegateModel.rootIndex = modelIndex;
                    } else {
                        toggleCurrentIndex();
                    }
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onLongPressed: toggleCurrentIndex()
            }
            TapHandler {
                acceptedDevices: PointerDevice.TouchScreen
                onLongPressed: contextMenu.open()
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onTapped: contextMenu.open()
            }
            Menu {
                id: contextMenu
                MenuItem {
                    text: qsTr("Refresh")
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    onClicked: delegateModel.model.fetchMore(delegateModel.modelIndex(index))
                }
                MenuItem {
                    text: checkStateData !== Qt.Checked ? qsTr("Select") : qsTr("Deselect")
                    icon.width: app.iconSize
                    icon.height: app.iconSize
                    onClicked: toggleCurrentIndex()
                }
            }
            function toggleCurrentIndex() {
                delegateModel.model.triggerAction("toggle-selection-single", delegateModel.modelIndex(index))
            }
        }
    }
    ListView {
        id: listView
        anchors.fill: parent
        ScrollIndicator.vertical: ScrollIndicator { }
        model: delegateModel
    }

    required property string dirName
    required property string dirId

    function back() {
        if (delegateModel.rootIndex.valid) {
            delegateModel.rootIndex = delegateModel.parentModelIndex();
            return true;
        } else {
            return false;
        }
    }
}
