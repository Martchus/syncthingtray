import QtQuick 2.0
import org.kde.plasma.components 3.0 as PlasmaComponents3

MouseArea {
    property alias interval: timer.interval
    property alias tooltip: tooltip.text
    hoverEnabled: true
    Timer {
        id: timer
        interval: 1000
        running: parent.containsMouse && parent.tooltip.length !== 0
        onTriggered: tooltip.open()
    }
    PlasmaComponents3.ToolTip {
        id: tooltip
    }
}
