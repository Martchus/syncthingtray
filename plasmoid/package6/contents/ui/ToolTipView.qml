import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

RowLayout {
    spacing: Kirigami.Units.smallSpacing

    PlasmaCore.IconItem {
        id: tooltipIcon
        source: plasmoid.statusIcon
        Layout.alignment: Qt.AlignCenter
        visible: true
        implicitWidth: Kirigami.Units.iconSizes.large
        Layout.topMargin: Kirigami.Units.smallSpacing
        Layout.leftMargin: Kirigami.Units.smallSpacing
        Layout.bottomMargin: Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        Layout.preferredWidth: implicitWidth
        Layout.preferredHeight: implicitWidth
    }

    ColumnLayout {
        Kirigami.Heading {
            id: tooltipMaintext
            level: 3
            elide: Text.ElideRight
            text: plasmoid.statusText
        }
        PlasmaComponents3.Label {
            id: tooltipSubtext
            text: plasmoid.additionalStatusText
            opacity: 0.6
        }
    }
}
