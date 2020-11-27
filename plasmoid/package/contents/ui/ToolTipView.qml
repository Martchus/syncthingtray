import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras

RowLayout {
    spacing: units.smallSpacing

    PlasmaCore.IconItem {
        id: tooltipIcon
        source: plasmoid.nativeInterface.statusIcon
        Layout.alignment: Qt.AlignCenter
        visible: true
        implicitWidth: units.iconSizes.large
        Layout.topMargin: units.smallSpacing
        Layout.leftMargin: units.smallSpacing
        Layout.bottomMargin: units.smallSpacing
        Layout.rightMargin: units.smallSpacing
        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitWidth
    }

    ColumnLayout {
        PlasmaExtras.Heading {
            id: tooltipMaintext
            level: 3
            elide: Text.ElideRight
            text: plasmoid.toolTipMainText
        }
        PlasmaComponents3.Label {
            id: tooltipSubtext
            text: plasmoid.toolTipSubText
            opacity: 0.6
        }
    }
}
