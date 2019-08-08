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
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            opacity: 0.7
        }
        PlasmaComponents.Label {
            id: label
        }
    }

    ToolTipTrigger {
        id: tooltipTrigger
        anchors.fill: layout
    }
}
