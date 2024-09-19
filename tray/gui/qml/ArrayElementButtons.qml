import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    visible: rowData.isArray
    RoundButton {
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        display: AbstractButton.IconOnly
        text: qsTr("Move down")
        ToolTip.text: text
        ToolTip.visible: hovered || pressed
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        icon.source: app.faUrlBase + "angle-down"
        icon.width: 20
        icon.height: 20
        onClicked: page.swapObjects(rowData, 1)
    }
    RoundButton {
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        display: AbstractButton.IconOnly
        text: qsTr("Move up")
        ToolTip.text: text
        ToolTip.visible: hovered || pressed
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        icon.source: app.faUrlBase + "angle-up"
        icon.width: 20
        icon.height: 20
        onClicked: page.swapObjects(rowData, -1)
    }
    RoundButton {
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        display: AbstractButton.IconOnly
        text: qsTr("More options")
        ToolTip.text: text
        ToolTip.visible: hovered || pressed
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        icon.source: app.faUrlBase + "ellipsis-v"
        icon.width: 20
        icon.height: 20
        onClicked: menu.popup()
    }
    Menu {
        id: menu
        MenuItem {
            text: qsTr("Remove")
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            icon.width: app.iconSize
            icon.height: app.iconSize
            icon.source: app.faUrlBase + "minus"
            onClicked: page.removeObjects(rowData, 1)
        }
        MenuItem {
            text: qsTr("Insert after")
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            icon.width: app.iconSize
            icon.height: app.iconSize
            icon.source: app.faUrlBase + "plus"
            onClicked: page.showNewValueDialog(rowData.index)
        }
    }
    required property var page
    required property var rowData
}
