import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: page
    title: {
        const mainTitle = qsTr("Remote/global tree of \"%1\"").arg(dirName);
        return path.length > 0 ? `${mainTitle}\n${path}` : mainTitle;
    }
    Component.onCompleted: connections.onSelectionActionsChanged()

    DelegateModel {
        id: delegateModel
        model: page.model
        delegate: ItemDelegate {
            id: itemDelegate
            width: listView.width
            contentItem: RowLayout {
                Icon {
                    source: decorationData
                }
                ColumnLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: textData
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.fillWidth: true
                        text: details
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                }
                CheckBox {
                    visible: checkable && page.model.selectionModeEnabled
                    checkState: checkStateData ?? Qt.Unchecked
                    onClicked: itemDelegate.toggle()
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onTapped: {
                    const modelIndex = delegateModel.modelIndex(index);
                    if (delegateModel.model.hasChildren(modelIndex)) {
                        delegateModel.rootIndex = modelIndex;
                        listView.positionViewAtBeginning();
                    } else {
                        itemDelegate.toggle();
                    }
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onLongPressed: {
                    itemDelegate.toggle();
                    App.performHapticFeedback();
                }
            }
            TapHandler {
                acceptedDevices: PointerDevice.TouchScreen
                onLongPressed: {
                    contextMenu.popup();
                    App.performHapticFeedback();
                }
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onTapped: contextMenu.popup()
            }
            Menu {
                id: contextMenu
                popupType: App.nativePopups ? Popup.Native : Popup.Item
                Instantiator {
                    model: actions
                    delegate: MenuItem {
                        text: actionNames[index]
                        onTriggered: itemDelegate.triggerAction(modelData)
                    }
                    onObjectAdded: (index, object) => contextMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => contextMenu.removeItem(object)
                }
            }
            function triggerAction(action) {
                delegateModel.model.triggerAction(action, delegateModel.modelIndex(index));
            }
            function toggle() {
                itemDelegate.triggerAction("toggle-selection-single");
            }
        }
    }
    CustomListView {
        id: listView
        anchors.fill: parent
        model: delegateModel
    }
    Instantiator {
        id: modelActionsInstantiator
        delegate: Action {
            text: modelData.text ?? "?"
            onTriggered: modelData.trigger()
        }
        onObjectAdded: (index, object) => page.modelActions.splice(index, 0, object)
    }
    CustomDialog {
        id: confirmActionDialog
        contentItem: ColumnLayout {
            Label {
                id: messageLabel
                Layout.fillWidth: true
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                TextArea {
                    id: diffTextArea
                    readOnly: true
                }
            }
        }
        onAccepted: action?.trigger()
        onRejected: action?.dismiss()
        property var action
        property var diffHighlighter: App.createDiffHighlighter(diffTextArea.textDocument.textDocument)
        property alias message: messageLabel.text
        property alias diff: diffTextArea.text
    }
    Connections {
        id: connections
        target: page.model
        function onSelectionActionsChanged() {
            page.modelActions = [];
            modelActionsInstantiator.model = page.model.selectionActions;
            page.extraActions = page.modelActions;
        }
        function onActionNeedsConfirmation(action, message, diff) {
            confirmActionDialog.title = action.text;
            confirmActionDialog.action = action;
            confirmActionDialog.message = message;
            confirmActionDialog.diff = diff;
            confirmActionDialog.open();
        }
    }

    required property string dirName
    required property string dirId
    property var model: App.createFileModel(dirId, listView)
    property string path: model.path(delegateModel.rootIndex)
    property var modelActions: []
    property var extraActions: []

    function back() {
        const rootIndex = delegateModel.rootIndex;
        const isValid = rootIndex.valid;
        if (isValid) {
            const parentRow = rootIndex.row;
            delegateModel.rootIndex = delegateModel.parentModelIndex();
            listView.positionViewAtIndex(parentRow, ListView.Center);
        }
        return isValid;
    }
}
