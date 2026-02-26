import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

CustomDialog {
    Material.primary: Material.LightBlue
    Material.accent: Material.LightBlue
    title: meta.title
    implicitWidth: 500
    contentItem: ColumnLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Do you want to open \"%1\"?").arg(currentUrl)
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            ForkAwesomeIcon {
                iconName: "exclamation-triangle"
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Android may decide to stop the current activity to free memory. So when going back to the Syncthing App, the app UI might be reset and unsaved changes be lost.")
                font.weight: Font.Light
                wrapMode: Text.WordWrap
            }
        }
    }
    onAccepted: App.openUrlExternally(currentUrl)
    property url currentUrl
}
