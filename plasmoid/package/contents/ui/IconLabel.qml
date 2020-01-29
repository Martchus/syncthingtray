import QtQuick 2.0
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    property alias iconSource: iconItem.source
    property alias iconOpacity: iconItem.opacity
    property alias text: label.text
    property alias tooltip: tooltipTrigger.tooltip

    implicitWidth: layout.implicitWidth
    implicitHeight: layout.implicitHeight

    RowLayout {
        id: layout

        PlasmaCore.IconItem {
            id: iconItem
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            Layout.maximumWidth: units.iconSizes.small
            Layout.maximumHeight: units.iconSizes.small
            opacity: 0.7
            Rectangle {
                color: "red"
                anchors.top: parent.top
                anchors.left: parent.left
                width: parent.paintedWidth
                height: parent.paintedHeight
            }
        }
        PlasmaComponents.Label {
            id: label
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }

    ToolTipTrigger {
        id: tooltipTrigger
        anchors.fill: layout
    }
}
