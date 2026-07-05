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
    visible: SyncthingData.connection.hasState
    contentItem: StatisticsRow {
        id: row
        dense: false
    }
    property alias stats: row.stats
    property alias iconName: row.iconName
    property alias labelText: row.labelText
}
