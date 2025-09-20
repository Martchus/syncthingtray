import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

CustomDialog {
    id: discardChangesDialog
    Material.primary: Material.LightBlue
    Material.accent: Material.LightBlue
    title: meta.title
    contentItem: Label {
        Layout.fillWidth: true
        text: qsTr("Do you really want to go back without applying changes?")
        wrapMode: Text.WordWrap
    }
    onAccepted: pageStack.pop(true)
    required property Meta meta
    required property PageStack pageStack
}
