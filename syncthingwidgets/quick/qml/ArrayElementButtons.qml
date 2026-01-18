import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    visible: rowData.isArray
    IconOnlyButton {
        text: qsTr("Move down")
        icon.source: App.faUrlBase + "angle-down"
        onClicked: page.swapObjects(rowData, 1)
    }
    IconOnlyButton {
        text: qsTr("Move up")
        icon.source: App.faUrlBase + "angle-up"
        onClicked: page.swapObjects(rowData, -1)
    }
    IconOnlyButton {
        id: menuButton
        text: qsTr("More options")
        icon.source: App.faUrlBase + "ellipsis-v"
        onClicked: menu.showCenteredIn(menuButton)
        CustomMenu {
            id: menu
            MenuItem {
                text: qsTr("Remove")
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                icon.width: App.iconSize
                icon.height: App.iconSize
                icon.source: App.faUrlBase + "minus"
                onClicked: page.removeObjects(rowData, 1)
            }
            MenuItem {
                text: qsTr("Insert before")
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                icon.width: App.iconSize
                icon.height: App.iconSize
                icon.source: App.faUrlBase + "plus"
                onClicked: page.showNewValueDialog(rowData.index)
            }
        }
    }
    required property var page
    required property var rowData
}
