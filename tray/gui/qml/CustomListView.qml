import QtQuick
import QtQuick.Controls.Material

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
    ScrollIndicator.vertical: ScrollIndicator { }
}
