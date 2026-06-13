import QtQuick
import QtQuick.Controls.Material

import Main

Dialog {
    id: dialog
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    popupType: Utils.popupType
    width: Math.min(popupType === Popup.Item ? Math.max(0, parent.width - additionalSpacing - leftMargin - rightMargin) : implicitWidth, 800)
    height: Math.min(implicitHeight, Math.max(0, parent.height - additionalSpacing - topMargin - bottomMargin))
    topMargin: parent?.SafeArea.margins.top ?? 0
    leftMargin: parent?.SafeArea.margins.left ?? 0
    rightMargin: parent?.SafeArea.margins.right ?? 0
    bottomMargin: parent?.SafeArea.margins.bottom ?? 0
    standardButtons: Dialog.Yes | Dialog.No
    modal: true
    onOpened: QuickUI.addDialog(dialog)
    onClosed: QuickUI.removeDialog(dialog)
    property int additionalSpacing: 20
}
