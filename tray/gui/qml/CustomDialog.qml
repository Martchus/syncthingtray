import QtQuick
import QtQuick.Controls.Material

import Main

Dialog {
    id: dialog
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    popupType: App.nativePopups ? Popup.Native : Popup.Item
    width: Math.min(popupType === Popup.Item ? parent.width - 20 : implicitWidth, 800)
    standardButtons: Dialog.Yes | Dialog.No
    modal: true
    onOpened: App.addDialog(dialog)
    onClosed: App.removeDialog(dialog)
}
