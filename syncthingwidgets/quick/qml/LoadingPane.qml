import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Pane {
    height: visible ? implicitHeight : 0
    contentItem: RowLayout {
        Item {
            Layout.fillWidth: true
        }
        BusyIndicator {
            Layout.preferredWidth: QuickUI.iconSize * 2
            Layout.preferredHeight: Layout.preferredWidth
        }
        Label {
            text: qsTr("Loading …")
            elide: Text.ElideRight
            font.weight: Font.Light
        }
        Item {
            Layout.fillWidth: true
        }
    }
}
