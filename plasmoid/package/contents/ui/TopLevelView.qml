import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras

ListView {
    anchors.fill: parent
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentHeight > height
    keyNavigationEnabled: true
    keyNavigationWraps: true
    currentIndex: -1

    highlightMoveDuration: 0
    highlightResizeDuration: 0
    highlight: PlasmaComponents.Highlight { }
}
