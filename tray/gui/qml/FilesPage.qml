import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
                    app.performHapticFeedback();
                }
            }
            TapHandler {
                acceptedDevices: PointerDevice.TouchScreen
                onLongPressed: {
                    contextMenu.open();
                    app.performHapticFeedback();
                }
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onTapped: contextMenu.open()
            }
            Menu {
                id: contextMenu
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
    ListView {
        id: listView
        anchors.fill: parent
        ScrollIndicator.vertical: ScrollIndicator { }
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
    Dialog {
        id: confirmActionDialog
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: parent.width - 20
        height: parent.height - 20
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
        property var diffHighlighter: app.createDiffHighlighter(diffTextArea.textDocument.textDocument)
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
    property var model: app.createFileModel(dirId, listView)
    property string path: model.path(delegateModel.rootIndex)
    property var modelActions: []
    property var extraActions: []

    function back() {
        const isValid = delegateModel.rootIndex.valid;
        if (isValid) {
            delegateModel.rootIndex = delegateModel.parentModelIndex();
        }
        return isValid;
    }
}
