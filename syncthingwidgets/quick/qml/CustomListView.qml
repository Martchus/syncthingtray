import QtQuick
import QtQuick.Controls.Material

import Main

ListView {
    id: listView
    activeFocusOnTab: true
    keyNavigationEnabled: true
    boundsMovement: Flickable.StopAtBounds
    boundsBehavior: Flickable.DragAndOvershootBounds
    transformOrigin: listView.verticalOvershoot >= 0 ? Item.Top : Item.Bottom
    transform: Scale {
        origin.y: listView.verticalOvershoot > 0 ? listView.height : 0
        yScale: 1 + Math.log(Math.abs(listView.verticalOvershoot) + 1) * 0.01
    }
    Component {
        id: scrollIndicator
        ScrollIndicator {}
    }
    Component {
        id: scrollBar
        ScrollBar {}
    }
    ScrollIndicator.vertical: !QuickUI.desktop ? scrollIndicator.createObject(listView) : null
    ScrollBar.vertical: QuickUI.desktop ? scrollBar.createObject(listView) : null
}
