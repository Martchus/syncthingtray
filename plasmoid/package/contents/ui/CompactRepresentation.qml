import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import martchus.syncthingplasmoid 0.6 as SyncthingPlasmoid

MouseArea {
    Layout.fillWidth: false
    hoverEnabled: true
    onClicked: plasmoid.expanded = !plasmoid.expanded

    PlasmaCore.IconItem {
        id: icon
        anchors.fill: parent
        source: plasmoid.nativeInterface.statusIcon
        active: parent.containsMouse
    }
}
