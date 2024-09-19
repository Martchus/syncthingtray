import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

ItemDelegate {
    id: mainDelegate
    width: mainView.width
    //onClicked: detailsView.visible = !detailsView.visible
    contentItem: ColumnLayout {
        RowLayout {
            spacing: 10
            Icon {
                source: modelData.statusIcon
                width: 24
                height: 24
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
                RoundButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    display: AbstractButton.IconOnly
                    text: modelData.text
                    visible: buttonRepeater.visible
                    enabled: modelData.enabled
                    hoverEnabled: true
                    ToolTip.visible: hovered || pressed
                    ToolTip.text: text
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    icon.source: modelData.icon.source
                    icon.width: 20
                    icon.height: 20
                    onClicked: modelData.trigger(source)
                    required property Action modelData
                }
            }
            RoundButton {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                visible: !buttonRepeater.visible || mainDelegate.extraActions.length > 0
                display: AbstractButton.IconOnly
                text: qsTr("More actions")
                hoverEnabled: true
                ToolTip.visible: hovered || pressed
                ToolTip.text: text
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                icon.source: app.faUrlBase + "ellipsis-v"
                icon.width: 20
                icon.height: 20
                onClicked: menu.popup()
            }
            Menu {
                id: menu
                Instantiator {
                    model: mainDelegate.actions
                    delegate: MenuItem {
                        required property Action modelData
                        text: modelData.text
                        visible: modelData.enabled
                        icon.source: modelData.icon.source
                        icon.width: app.iconSize
                        icon.height: app.iconSize
                        onTriggered: modelData.trigger(source)
                    }
                    onObjectAdded: (index, object) => menu.insertItem(index, object)
                    onObjectRemoved: (index, object) => menu.removeItem(object)
                }
                Instantiator {
                    model: mainDelegate.extraActions
                    delegate: MenuItem {
                        required property Action modelData
                        text: modelData.text
                        visible: modelData.enabled
                        icon.source: modelData.icon.source
                        icon.width: app.iconSize
                        icon.height: app.iconSize
                        onTriggered: modelData.trigger(source)
                    }
                    onObjectAdded: (index, object) => menu.insertItem(index, object)
                    onObjectRemoved: (index, object) => menu.removeItem(object)
                }
            }
        }
        DetailsListView {
            id: detailsView
            mainView: mainDelegate.mainView
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: detailsView.visible = !detailsView.visible
        onLongPressed: menu.popup()
    }
    TapHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus
        acceptedButtons: Qt.RightButton
        onTapped: menu.popup()
    }

    required property var modelData
    required property ListView mainView
    readonly property bool breakpoint: mainView.width > 500
    property list<Action> actions
    property list<Action> extraActions
}
