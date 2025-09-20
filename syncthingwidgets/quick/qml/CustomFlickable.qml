import QtQuick
import QtQuick.Controls.Material

Flickable {
    id: flickable
    boundsMovement: Flickable.StopAtBounds
    boundsBehavior: Flickable.DragAndOvershootBounds
    transformOrigin: flickable.verticalOvershoot >= 0 ? Item.Top : Item.Bottom
    transform: Scale {
        origin.y: flickable.verticalOvershoot > 0 ? flickable.height : 0
        yScale: 1 + Math.log(Math.abs(flickable.verticalOvershoot) + 1) * 0.01
    }
    ScrollIndicator.vertical: ScrollIndicator { }
}
