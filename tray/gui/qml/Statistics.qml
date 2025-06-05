import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ItemDelegate {
    id: itemDelegate
    Layout.fillWidth: true
    ToolTip.visible: pressed
    ToolTip.text: qsTr("%1 files, %2 dirs, ~ %3").arg(stats.files).arg(stats.dirs).arg(stats.bytesAsString)
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    hoverEnabled: true
    visible: App.connection.hasState
    contentItem: RowLayout {
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
            }
            GridLayout {
                Layout.fillWidth: true
                columns: itemDelegate.width > 300 ? 6 : 2
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
    }
    required property var stats
    property alias iconName: icon.iconName
    property alias labelText: label.text
}
