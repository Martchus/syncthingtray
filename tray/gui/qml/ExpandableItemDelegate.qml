import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

ItemDelegate {
    id: mainDelegate
    width: mainView.width
    onClicked: detailsView.visible = !detailsView.visible

    contentItem: ColumnLayout {
        RowLayout {
            spacing: 10
            Icon {
                source: modelData.statusIcon
                width: 24
                height: 24
            }
            Label {
                Layout.fillWidth: true
                text: modelData.name
                elide: Text.ElideRight
                font.weight: Font.Medium
            }
            Label {
                text: modelData.statusString ?? '?'
                color: modelData.statusColor ?? palette.text
            }
            QtObject {
                id: source
                property int row: modelData.index
                property int col: 0
            }
            Repeater {
                model: mainDelegate.actions
                RoundButton {
                    required property Action modelData
                    enabled: modelData.enabled
                    hoverEnabled: true
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    ToolTip.visible: hovered || pressed
                    ToolTip.text: modelData.text
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    icon.source: modelData.icon.source
                    icon.width: 20
                    icon.height: 20
                    onClicked: modelData.trigger(source)
                }
            }
            RoundButton {
                visible: mainDelegate.extraActions.length > 0
                hoverEnabled: true
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                ToolTip.visible: hovered || pressed
                ToolTip.text: qsTr("More actions")
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                icon.source: app.faUrlBase + "bars"
                icon.width: 20
                icon.height: 20
                onClicked: menu.popup()
            }
            Menu {
                id: menu
                Instantiator {
                    id: menuInstantiator
                    model: mainDelegate.extraActions
                    delegate: MenuItem {
                        required property Action modelData
                        text: modelData.text
                        enabled: modelData.enabled
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

    required property var modelData
    required property ListView mainView
    property list<Action> actions
    property list<Action> extraActions
}
