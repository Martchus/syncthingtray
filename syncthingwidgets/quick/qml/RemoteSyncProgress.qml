import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    spacing: 15
    ForkAwesomeIcon {
        Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
        iconName: "upload"
    }
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Remote sync progress (of connected devices)")
            wrapMode: Text.Wrap
            font.weight: Font.Medium
        }
        RowLayout {
            Layout.fillWidth: true
            ProgressBar {
                Layout.fillWidth: true
                id: remoteProgressBar
                from: 0
                to: 100
                value: remoteCompletion.percentage
            }
            Label {
                text: remoteProgressBar.position >= 1 ? qsTr("Up to Date") : (Number.isNaN(remoteCompletion.percentage) ? qsTr("Not available") : qsTr("%1 %").arg(Math.round(remoteCompletion.percentage)))
                font.weight: Font.Light
            }
        }
    }
    property var remoteCompletion: SyncthingData.connection.overallRemoteCompletion
}
