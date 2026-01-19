import QtQuick
import QtQuick.Controls.Material

import Main

Dialog {
    id: dialog
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    popupType: App.windowPopups ? Popup.Window : Popup.Item
    width: Math.min(popupType === Popup.Item ? Math.max(0, parent.width - additionalSpacing - leftMargin - rightMargin) : implicitWidth, 800)
    height: Math.min(implicitHeight, Math.max(0, parent.height - additionalSpacing - topMargin - bottomMargin))
    topMargin: parent.SafeArea.margins.top
    leftMargin: parent.SafeArea.margins.left
    rightMargin: parent.SafeArea.margins.right
    bottomMargin: parent.SafeArea.margins.bottom
    standardButtons: Dialog.Yes | Dialog.No
    modal: true
    onOpened: App.addDialog(dialog)
    onClosed: App.removeDialog(dialog)
    property int additionalSpacing: 20
}
