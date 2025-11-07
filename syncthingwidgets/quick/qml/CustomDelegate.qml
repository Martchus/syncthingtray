import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ItemDelegate {
    Layout.fillWidth: true
    contentItem: RowLayout {
        spacing: 15
        ForkAwesomeIcon {
            id: icon
        }
        Label {
            id: label
            Layout.fillWidth: true
            elide: Text.ElideRight
            font.weight: Font.Medium
        }
    }
    property alias labelText: label.text
    property alias iconName: icon.iconName
}
