import QtQuick
import QtQuick.Controls.Material

import Main

Dialog {
    id: dialog
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    popupType: App.nativePopups ? Popup.Native : Popup.Item
    width: Math.min(popupType === Popup.Item ? Math.max(0, parent.width - margin - parent.SafeArea.margins.left - parent.SafeArea.margins.right) : implicitWidth, 800)
    height: Math.min(implicitHeight, Math.max(0, parent.height - margin - parent.SafeArea.margins.top - parent.SafeArea.margins.bottom))
    standardButtons: Dialog.Yes | Dialog.No
    modal: true
    onOpened: App.addDialog(dialog)
    onClosed: App.removeDialog(dialog)
    property int margin: 20
}
