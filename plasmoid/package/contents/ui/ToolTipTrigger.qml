import QtQuick 2.0
import QtQuick.Controls.Private 1.0 as ControlsPrivate // Why is such a basic thing as a tooltip private?! Let's be evil and just use it.

MouseArea {
    property alias interval: timer.interval
    property string tooltip: ""
    hoverEnabled: true
    Timer {
        id: timer
        interval: 1000
        running: parent.containsMouse && parent.tooltip.length !== 0
        onTriggered: ControlsPrivate.Tooltip.showText(parent, Qt.point(parent.mouseX, parent.mouseY), parent.tooltip)
    }
}
