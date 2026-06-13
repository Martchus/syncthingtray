import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    spacing: 15
    ForkAwesomeIcon {
        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
        iconName: "download"
    }
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Local sync progress")
            wrapMode: Text.Wrap
            font.weight: Font.Medium
        }
        RowLayout {
            Layout.fillWidth: true
            ProgressBar {
                Layout.fillWidth: true
                id: progressBar
                from: 0
                to: 100
                value: stats.completionPercentage
            }
            Label {
                text: progressBar.position >= 1 ? qsTr("Up to Date") : qsTr("%1 %, %2 remaining").arg(Math.floor(progressBar.position * 100)).arg(stats.needed.bytesAsString)
                font.weight: Font.Light
            }
        }
    }
    readonly property var stats: SyncthingData.connection.overallDirStatistics
}
