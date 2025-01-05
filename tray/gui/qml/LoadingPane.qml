import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Pane {
    height: visible ? implicitHeight : 0
    contentItem: RowLayout {
        BusyIndicator {
            Layout.preferredWidth: App.iconSize * 2
            Layout.preferredHeight: Layout.preferredWidth
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Loading …")
            elide: Text.ElideRight
            font.weight: Font.Light
        }
    }
}
