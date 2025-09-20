import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

CustomDialog {
    id: closeDialog
    Material.primary: Material.LightBlue
    Material.accent: Material.LightBlue
    standardButtons: Dialog.NoButton
    title: meta.title
    contentItem: Label {
        Layout.fillWidth: true
        text: qsTr("Do you want to shutdown Syncthing and quit the app? You can also just quit the app and keep Syncthing running in the background.")
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
