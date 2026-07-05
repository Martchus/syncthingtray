import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    id: row
    spacing: 15
    ForkAwesomeIcon {
        id: icon
        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
    }
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            id: label
            Layout.fillWidth: true
            font.weight: Font.Medium
            elide: Text.ElideRight
            visible: !dense
        }
        GridLayout {
            Layout.fillWidth: true
            columns: row.parent.width > 300 ? 6 : 2
            ForkAwesomeIcon {
                iconName: "file-o"
            }
            Label {
                text: stats.files
                font.weight: Font.Light
            }
            ForkAwesomeIcon {
                iconName: "folder-o"
            }
            Label {
                text: stats.dirs
                font.weight: Font.Light
            }
            ForkAwesomeIcon {
                iconName: "hdd-o"
            }
            Label {
                text: stats.bytesAsString
                font.weight: Font.Light
            }
        }
    }
    property bool dense: true
    required property var stats
    property alias iconName: icon.iconName
    property alias labelText: label.text
}
