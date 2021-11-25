import QtQuick 2.8
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaComponents3.TabButton {
    id: root
    contentItem: RowLayout {
        spacing: label.visible ? units.smallSpacing : 0
        PlasmaCore.ColorScope.inherit: true
        Image {
            id: image
            Layout.preferredHeight: height
            source: root.icon.source
            height: units.iconSizes.small
            fillMode: Image.PreserveAspectFit
        }
        PlasmaComponents3.Label {
            id: label
            visible: text.length > 0
            text: root.text
            font: root.parent.font
            color: PlasmaCore.ColorScope.textColor
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
