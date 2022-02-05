import QtQuick 2.8
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaComponents3.ToolButton {
    id: root
    Layout.fillHeight: true
    contentItem: Grid {
        columns: 2
        columnSpacing: label.visible ? units.smallSpacing : 0
        verticalItemAlignment: Grid.AlignVCenter
        PlasmaCore.ColorScope.inherit: true
        Image {
            source: root.icon.source
            cache: false
            height: parent.height
            fillMode: Image.PreserveAspectFit
        }
        PlasmaComponents3.Label {
            id: label
            visible: text.length > 0
            text: root.text
            color: PlasmaCore.ColorScope.textColor
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}

