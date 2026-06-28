import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ItemDelegate {
    id: mainDelegate
    width: mainView.width - (mainView.ScrollBar?.vertical ? mainView.ScrollBar.vertical.width : 0)
    activeFocusOnTab: true
    Keys.onReturnPressed: (event) => detailsView.visible = !detailsView.visible
    Keys.onMenuPressed: (event) => menu.showCenteredIn(menuButton)
    contentItem: ColumnLayout {
        RowLayout {
            spacing: 10 * QuickUI.densityScale
            Icon {
                id: statusIcon
                source: modelData.statusIcon
                Layout.preferredWidth: Layout.preferredHeight
                Layout.preferredHeight: Math.min(32, mainDelegate.breakpoint ? nameLabel.height : nameLabel.height + statusLabel.height, )
                visible: QuickUI.syncthingIconsVisible
            }
            GridLayout {
                Layout.fillWidth: true
                columns: mainDelegate.breakpoint ? 2 : 1
                columnSpacing: 10
                rowSpacing: 0
                Label {
                    Layout.fillWidth: true
                    id: nameLabel
                    text: modelData.name
                    elide: Text.ElideRight
                    font.weight: Font.Medium
                }
                Label {
                    id: statusLabel
                    text: modelData.statusString ?? '?'
                    color: modelData.statusColor ?? palette.text
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }
            }
            Icon {
                id: storageIcon
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                source: modelData.storageIcon
                MouseArea {
                    anchors.fill: parent
                    onPressAndHold: {
                        storageToolTip.show(modelData.storageTooltip);
                        QuickUI.performHapticFeedback();
                    }
                }
                ToolTip {
                    id: storageToolTip
                }
            }
            QtObject {
                id: source
                property int row: modelData.index
                property int col: 0
            }
            Repeater {
                id: buttonRepeater
                visible: mainDelegate.breakpoint || (buttonRepeater.count + mainDelegate.extraActions.length) === 1
                model: mainDelegate.actions
                IconOnlyButton {
                    text: modelData.text
                    visible: buttonRepeater.visible
                    enabled: modelData.enabled
                    icon.source: modelData.icon.source
                    onClicked: modelData.trigger(source)
                    required property Action modelData
                }
            }
            IconOnlyButton {
                id: menuButton
                visible: !buttonRepeater.visible || mainDelegate.extraActions.length > 0
                text: qsTr("More actions")
                icon.source: QuickUI.faUrlBase + "ellipsis-v"
                onClicked: menu.showCenteredIn(menuButton)
                CustomMenu {
                    id: menu
                    MenuItemInstantiator {
                        menu: menu
                        model: mainDelegate.actions
                    }
                    MenuSeparatorInstantiator {
                        menu: menu
                    }
                    MenuItemInstantiator {
                        menu: menu
                        model: mainDelegate.extraActions
                    }
                }
            }
        }
        DetailsListView {
            id: detailsView
            mainDelegate: mainDelegate
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: {
            detailsView.visible = !detailsView.visible;
            mainDelegate.forceActiveFocus(Qt.MouseFocusReason);
        }
        onLongPressed: {
            QuickUI.performHapticFeedback();
            menu.showCenteredIn(menuButton);
        }
    }
    TapHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
        acceptedButtons: Qt.RightButton
        onTapped: menu.showCenteredIn(menuButton)
    }

    required property var modelData
    required property ListView mainView
    readonly property bool breakpoint: mainView.width > (500 * Math.max(0.8, QuickUI.densityScale))
    readonly property real statusIconWidth: statusIcon.visible ? statusIcon.width : 0
    property list<Action> actions
    property list<Action> extraActions
}
