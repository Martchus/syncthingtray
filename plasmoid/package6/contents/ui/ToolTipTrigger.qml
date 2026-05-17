import QtQuick 2.0
import org.kde.plasma.components 3.0 as PlasmaComponents3

MouseArea {
    id: mouseArea
    property alias interval: tooltip.delay
    property alias tooltip: tooltip.text
    hoverEnabled: true
    onContainsMouseChanged: tooltip.text.length > 0 && mouseArea.containsMouse ? tooltip.open() : tooltip.close()
    PlasmaComponents3.ToolTip {
        id: tooltip
        delay: 1000
        parent: detailLabel
    }
}
