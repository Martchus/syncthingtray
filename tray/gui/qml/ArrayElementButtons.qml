import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    visible: rowData.isArray
    RoundButton {
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        icon.width: 20
        icon.height: 20
        icon.source: app.faUrlBase + "angle-down"
        onClicked: page.swapObjects(rowData, 1)
    }
    RoundButton {
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        icon.width: 20
        icon.height: 20
        icon.source: app.faUrlBase + "angle-up"
        onClicked: page.swapObjects(rowData, -1)
    }
    RoundButton {
        Layout.preferredWidth: 36
        Layout.preferredHeight: 36
        icon.width: 20
        icon.height: 20
        icon.source: app.faUrlBase + "ellipsis-v"
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