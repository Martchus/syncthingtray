import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami

MouseArea {
    property bool wasExpanded
    Layout.fillWidth: false
    hoverEnabled: true

    onPressed: wasExpanded = syncthingApplet.expanded
    onClicked: mouse => {
       if (mouse.button === Qt.MiddleButton) {
           Plasmoid.showWebUI();
       } else {
           syncthingApplet.expanded = !wasExpanded;
       }
    }

    Kirigami.Icon {
        id: icon
        anchors.fill: parent
        source: plasmoid.statusIcon
        active: parent.containsMouse
    }
}
