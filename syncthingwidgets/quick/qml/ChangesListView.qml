import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

CustomListView {
    id: mainView
    anchors.fill: parent
    model: DelegateModel {
        id: delegateModel
        model: SyncthingModels.changesModel
        delegate: ItemDelegate {
            width: mainView.width
            id: delegate
            onClicked: SyncthingModels.openPath(modelData.directoryId, modelData.path)
            onPressAndHold: SyncthingModels.copyPath(modelData.directoryId, modelData.path)
            contentItem: GridLayout {
                id: gridLayout
                columns: width < 500 ? 2 : 8
                columnSpacing: 10
                ForkAwesomeIcon {
                    id: firstIcon
                    iconName: (modelData.action === "deleted") ? ("trash-o") : (modelData.itemType === "file" ? "file-o" : "folder-o")
                }
                Label {
                    Layout.fillWidth: true
                    text: [modelData.directoryName || modelData.directoryId, modelData.path].join(": ")
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }
                ForkAwesomeIcon {
                    iconName: "calendar"
                }
                Label {
                    Layout.preferredWidth: Math.max(implicitWidth, parent.width / 5)
                    text: modelData.eventTime
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }
                Icon {
                    Layout.preferredWidth: QuickUI.iconSize
                    Layout.preferredHeight: QuickUI.iconSize
                    source: modelData.actionIcon
                    width: 16
                    height: 16
                }
                Label {
                    Layout.preferredWidth: Math.max(implicitWidth, parent.width / 5)
                    text: modelData.modifiedBy
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }
            }
            CustomMenu {
                id: menu
                MenuItemInstantiator {
                    menu: menu
                    model: delegate.actions
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onLongPressed: {
                    QuickUI.performHapticFeedback();
                    menu.showCenteredInRight(firstIcon);
                }
            }
            TapHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
                acceptedButtons: Qt.RightButton
                onTapped: menu.showCenteredInRight(firstIcon)
            }
            required property var modelData
            readonly property list<Action> actions: [
                Action {
                    text: qsTr("Open item")
                    icon.source: QuickUI.faUrlBase + "folder-open"
                    enabled: modelData.action !== "deleted"
                    onTriggered: SyncthingModels.openPath(SyncthingData.connection.fullPath(modelData.directoryId, modelData.path))
                },
                Action {
                    text: qsTr("Copy path")
                    icon.source: QuickUI.faUrlBase + "fa-files-o"
                    onTriggered: SyncthingModels.copyText(SyncthingData.connection.fullPath(modelData.directoryId, modelData.path))
                },
                Action {
                    text: qsTr("Copy device")
                    icon.source: QuickUI.faUrlBase + "sitemap"
                    onTriggered: SyncthingModels.copyText(modelData.modifiedBy)
                },
                Action {
                    text: qsTr("Copy folder")
                    icon.source: QuickUI.faUrlBase + "folder"
                    onTriggered: SyncthingModels.copyText(modelData.directoryId)
                }
            ]
        }
    }
    property alias delegateModel: delegateModel
}
