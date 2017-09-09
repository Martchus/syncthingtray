import QtQuick 2.7
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents

ListView {
    anchors.fill: parent
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentHeight > height
    keyNavigationEnabled: true
    keyNavigationWraps: true
    currentIndex: -1

    highlightMoveDuration: 0
    highlightResizeDuration: 0
    highlight: PlasmaComponents.Highlight {
    }
}
