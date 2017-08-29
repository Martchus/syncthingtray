import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

PlasmaComponents.ListItem {
    id: item
    property bool expanded: false
    enabled: true
    onClicked: expanded = !expanded
    onContainsMouseChanged: view.currentIndex = containsMouse ? index : -1
}
