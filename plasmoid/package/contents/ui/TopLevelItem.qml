import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ListItem {
    id: item
    property bool expanded: false
    enabled: true
    onClicked: expanded = !expanded
    onContainsMouseChanged: view.currentIndex = containsMouse ? index : -1
}
