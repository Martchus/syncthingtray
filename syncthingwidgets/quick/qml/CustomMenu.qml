import QtQuick.Controls.Material

import Main

Menu {
    id: menu
    popupType: App.nativePopups ? Popup.Native : Popup.Item
    topMargin: contentItem.Window.window.SafeArea.margins.top
    leftMargin: contentItem.Window.window.SafeArea.margins.left
    rightMargin: contentItem.Window.window.SafeArea.margins.right
    bottomMargin: contentItem.Window.window.SafeArea.margins.bottom
    function showCenteredIn(item) {
        menu.popup(item, item.width / 2 - menu.width, item.height / 2)
    }
}
