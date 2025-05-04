import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Dialog {
    id: closeDialog
    Material.primary: Material.LightBlue
    Material.accent: Material.LightBlue
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    popupType: App.nativePopups ? Popup.Native : Popup.Item
    width: Math.min(popupType === Popup.Item ? parent.width - 20 : implicitWidth, 800)
    standardButtons: Dialog.NoButton
    modal: true
    title: meta.title
    contentItem: Label {
        Layout.fillWidth: true
        text: qsTr("Do you want to shutdown Syncthing and quit the app? You can also just quit the app and keep Syncthing running in the backround.")
        wrapMode: Text.WordWrap
    }
    onAccepted: closeRequested()
    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: qsTr("Shutdown")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Background")
            flat: true
            onClicked: {
                closeDialog.close();
                App.minimize();
            }
            DialogButtonBox.buttonRole: DialogButtonBox.InvalidRole
        }
    }
    required property Meta meta
    signal closeRequested
}
