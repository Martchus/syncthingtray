import QtQuick 2.0
import QtQuick.Controls
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

Item {
    id: iconLabel
    property alias iconSource: iconItem.source
    property alias iconOpacity: iconItem.opacity
    property alias text: label.text
    property string tooltip

    implicitWidth: layout.implicitWidth
    implicitHeight: layout.implicitHeight

    RowLayout {
        id: layout

        Image {
            id: iconItem
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            height: 16
            cache: false
            fillMode: Image.PreserveAspectFit
        }
        PlasmaComponents3.Label {
            id: label
            ToolTip.delay: 1000
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onContainsMouseChanged: iconLabel.tooltip.length > 0 && mouseArea.containsMouse
                                        ? label.ToolTip.show(iconLabel.tooltip, 5000)
                                        : label.ToolTip.hide()
            }
        }
    }
}
