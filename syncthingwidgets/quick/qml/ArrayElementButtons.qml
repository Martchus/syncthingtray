import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    visible: rowData.isArray
    IconOnlyButton {
        text: qsTr("Move down")
        icon.source: QuickUI.faUrlBase + "angle-down"
        onClicked: page.swapObjects(rowData, 1)
    }
    IconOnlyButton {
        text: qsTr("Move up")
        icon.source: QuickUI.faUrlBase + "angle-up"
        onClicked: page.swapObjects(rowData, -1)
    }
    IconOnlyButton {
        id: menuButton
        text: qsTr("More options")
        icon.source: QuickUI.faUrlBase + "ellipsis-v"
        onClicked: menu.showCenteredIn(menuButton)
        CustomMenu {
            id: menu
            MenuItem {
                text: qsTr("Remove")
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                icon.width: QuickUI.iconSize
                icon.height: QuickUI.iconSize
                icon.source: QuickUI.faUrlBase + "minus"
                onClicked: page.removeObjects(rowData, 1)
            }
            MenuItem {
                text: qsTr("Insert before")
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                icon.width: QuickUI.iconSize
                icon.height: QuickUI.iconSize
                icon.source: QuickUI.faUrlBase + "plus"
                onClicked: page.showNewValueDialog(rowData.index)
            }
        }
    }
    required property var page
    required property var rowData
}
