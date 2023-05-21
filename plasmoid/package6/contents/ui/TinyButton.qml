import QtQuick 2.8
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

PlasmaComponents3.ToolButton {
    id: root

    property alias tooltip: tooltip.text

    Layout.fillHeight: true
    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium

    PlasmaComponents3.ToolTip {
        id: tooltip
    }
    contentItem: Image {
        source: root.icon.source
        cache: false
        height: parent.height
        fillMode: Image.PreserveAspectFit
    }
}
