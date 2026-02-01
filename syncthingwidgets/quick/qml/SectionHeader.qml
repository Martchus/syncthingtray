import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

RowLayout {
    spacing: 10
    width: parent.width
    Label {
        Layout.fillWidth: true
        Layout.margins: 15
        Layout.topMargin: 20
        Layout.bottomMargin: 10
        text: parent.section
        color: Material.accent
        elide: Text.ElideRight
        font.weight: Font.Medium
        wrapMode: Text.WordWrap
    }
    required property string section
}
