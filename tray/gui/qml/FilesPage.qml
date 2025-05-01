import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: page
    title: qsTr("Remote/global tree of \"%1\"").arg(dirName)
    Component.onCompleted: connections.onSelectionActionsChanged()

    DelegateModel {
        id: delegateModel
        model: page.model
        delegate: ItemDelegate {
            id: itemDelegate
            width: listView.width
            contentItem: RowLayout {
                Icon {
                    id: fileIcon
                    source: decorationData
                }
                ColumnLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: textData
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.fillWidth: true
                        text: details
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
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
                    contextMenu.show();
                    App.performHapticFeedback();
                }
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                onTapped: contextMenu.show()
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
                function show() {
                    contextMenu.popup(fileIcon, fileIcon.width / 2, fileIcon.height / 2)
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
        header: ItemDelegate {
            visible: path.length > 0
            height: visible ? implicitHeight : 0
            width: listView.width
            contentItem: RowLayout {
                ForkAwesomeIcon {
                    iconName: "folder-open-o"
                }
                Label {
                    Layout.fillWidth: true
                    text: path
                    elide: Label.ElideRight
                    wrapMode: Text.Wrap
                }
            }
            onClicked: page.back()
        }
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
            id: confirmActionLayout
            Label {
                id: messageLabel
                Layout.fillWidth: true
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
            }
            CustomListView {
                id: deletionsView
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(deletionsView.contentHeight, confirmActionLayout.height / 2)
                ScrollBar.vertical: ScrollBar { }
                ScrollIndicator.vertical: ScrollIndicator {
                    visible: false
                }
                clip: true
                model: ListModel {
                    id: deletionsModel
                }
                header: Label {
                    width: deletionsView.width
                    height: visible ? implicitHeight : 0
                    visible: deletionsView.model.count > 0
                    text: qsTr("Deletion of the following local files (will affect other devices unless ignored below!):")
                    font.weight: Font.Light
                    wrapMode: Text.WordWrap
                }
                delegate: CheckDelegate {
                    id: deletionDelegate
                    width: deletionsView.width
                    text: modelData.path
                    checkState: modelData.checked ? Qt.Checked : Qt.Unchecked
                    onCheckedChanged: deletionsModel.setProperty(modelData.index, "checked", deletionDelegate.checked)
                    required property var modelData
                }
            }
            Label {
                Layout.fillWidth: true
                visible: deletionsView.model.count > 0
                text: qsTr("Changes to ignore patterns:")
                font.weight: Font.Light
                wrapMode: Text.WordWrap
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
        onAccepted: {
            const localDeletions = [];
            const localDeletionCount = deletionsModel.count;
            for (let i = 0; i !== localDeletionCount; ++i) {
                const localDeletion = deletionsModel.get(i);
                if (localDeletion.checked) {
                    localDeletions.push(localDeletion.path);
                }
            }
            page.model.editLocalDeletionsFromVariantList(localDeletions);
            action?.trigger()
        }
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
        function onActionNeedsConfirmation(action, message, diff, localDeletions) {
            confirmActionDialog.title = action.text;
            confirmActionDialog.action = action;
            confirmActionDialog.message = message;
            confirmActionDialog.diff = diff;
            deletionsModel.clear();
            let index = 0;
            for (const path of localDeletions) {
                deletionsModel.append({path: path, checked: true, index: index++});
            }
            confirmActionDialog.open();
        }
    }
    Drawer {
        id: extraActionsDrawer
        width: window.width
        contentHeight: Math.min(drawerListView.contentHeight, window.height)
        edge: Qt.BottomEdge
        CustomListView {
            id: drawerListView
            anchors.fill: parent
            model: page.extraActions
            delegate: ItemDelegate {
                id: drawerDelegate
                width: drawerListView.width
                text: modelData.text
                enabled: modelData.enabled
                visible: enabled
                height: visible ? implicitHeight : 0
                onClicked: modelData?.trigger()
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: drawerDelegate.text
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                    }
                }
                required property Action modelData
            }
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

    function showExtraActions() {
        extraActionsDrawer.open();
        return true;
    }
}
