import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

ItemDelegate {
    id: mainDelegate
    width: mainView.width
    activeFocusOnTab: true
    Keys.onReturnPressed: (event) => detailsView.visible = !detailsView.visible
    Keys.onMenuPressed: (event) => menu.popup()
    contentItem: ColumnLayout {
        RowLayout {
            spacing: 10
            Icon {
                id: statusIcon
                source: modelData.statusIcon
            }
            GridLayout {
                Layout.fillWidth: true
                columns: mainDelegate.breakpoint ? 2 : 1
                columnSpacing: 10
                rowSpacing: 0
                Label {
                    Layout.fillWidth: true
                    text: modelData.name
                    elide: Text.ElideRight
                    font.weight: Font.Medium
                }
                Label {
                    text: modelData.statusString ?? '?'
                    color: modelData.statusColor ?? palette.text
                    elide: Text.ElideRight
                    font.weight: Font.Light
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
                visible: !buttonRepeater.visible || mainDelegate.extraActions.length > 0
                text: qsTr("More actions")
                icon.source: app.faUrlBase + "ellipsis-v"
                onClicked: menu.popup()
                Menu {
                    id: menu
                    popupType: app.nativePopups ? Popup.Native : Popup.Item
                    Instantiator {
                        model: mainDelegate.actions
                        delegate: MenuItem {
                            required property Action modelData
                            text: modelData.text
                            enabled: modelData.enabled
                            icon.source: modelData.icon.source
                            icon.width: app.iconSize
                            icon.height: app.iconSize
                            onTriggered: modelData.trigger(source)
                        }
                        onObjectAdded: (index, object) => object.enabled && menu.insertItem(index, object)
                        onObjectRemoved: (index, object) => menu.removeItem(object)
                    }
                    Instantiator {
                        model: mainDelegate.extraActions
                        delegate: MenuItem {
                            required property Action modelData
                            text: modelData.text
                            enabled: modelData.enabled
                            icon.source: modelData.icon.source
                            icon.width: app.iconSize
                            icon.height: app.iconSize
                            onTriggered: modelData.trigger(source)
                        }
                        onObjectAdded: (index, object) => object.enabled && menu.insertItem(index, object)
                        onObjectRemoved: (index, object) => menu.removeItem(object)
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
            app.performHapticFeedback();
            menu.popup();
        }
    }
    TapHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
        acceptedButtons: Qt.RightButton
        onTapped: menu.popup()
    }

    required property var modelData
    required property ListView mainView
    readonly property bool breakpoint: mainView.width > 500
    property alias statusIconWidth: statusIcon.width
    property list<Action> actions
    property list<Action> extraActions
}
