import QtQuick
import QtQuick.Controls.Material

import Main

Menu {
    id: menu
    popupType: App.windowPopups ? Popup.Window : Popup.Item
    topMargin: contentItem.Window?.window?.SafeArea?.margins.top ?? 0
    leftMargin: contentItem.Window?.window?.SafeArea?.margins.left ?? 0
    rightMargin: contentItem.Window?.window?.SafeArea?.margins.right ?? 0
    bottomMargin: contentItem.Window?.window?.SafeArea?.margins.bottom ?? 0
    Component.onCompleted: {
        if (contentItem.boundsMovement === undefined || contentItem.boundsBehavior === undefined) {
            return;
        }
        scale.origin.y = Qt.binding(() => contentItem.verticalOvershoot > 0 ? contentItem.height : 0);
        scale.yScale = Qt.binding(() => 1 + Math.log(Math.abs(contentItem.verticalOvershoot) + 1) * 0.01);
        contentItem.boundsMovement = Flickable.StopAtBounds;
        contentItem.boundsBehavior = Flickable.DragAndOvershootBounds;
        contentItem.transformOrigin = Qt.binding(() => contentItem.verticalOvershoot >= 0 ? Item.Top : Item.Bottom);
        contentItem.transform = scale;
    }
    property Scale transform: Scale {
        id: scale
    }
    function showCenteredIn(item) {
        menu.popup(item, item.width / 2 - menu.width, item.height / 2)
    }
    function showCenteredInRight(item) {
        menu.popup(item, item.width / 2, item.height / 2);
    }
}
