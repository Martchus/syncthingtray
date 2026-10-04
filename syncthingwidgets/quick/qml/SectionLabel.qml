import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Label {
    Layout.fillWidth: true
    Layout.margins: 16
    Layout.topMargin: 20
    Layout.bottomMargin: 10
    color: Material.accent
    elide: Text.ElideMiddle
    font.weight: Font.Medium
    wrapMode: Text.WordWrap
}
