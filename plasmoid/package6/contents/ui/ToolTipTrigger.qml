import QtQuick 2.0
import org.kde.plasma.components 3.0 as PlasmaComponents3

MouseArea {
    id: mouseArea
    property alias interval: timer.interval
    property alias tooltip: tooltip.text
    hoverEnabled: true
    onContainsMouseChanged: {
        if (!mouseArea.containsMouse) {
            tooltip.close()
        }
    }
    Timer {
        id: timer
        interval: 1000
        running: mouseArea.containsMouse && tooltip.text.length !== 0
        onTriggered: tooltip.open()
    }
    PlasmaComponents3.ToolTip {
        id: tooltip
    }
}
