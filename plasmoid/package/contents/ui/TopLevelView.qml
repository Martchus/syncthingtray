import QtQuick 2.7
import org.kde.plasma.components 2.0 as PlasmaComponents // for Highlight and DialogStatus.Closed (used with Menu and MenuItem)

ListView {
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentHeight > height
    keyNavigationEnabled: true
    keyNavigationWraps: true
    currentIndex: -1

    highlightMoveDuration: 0
    highlightResizeDuration: 0
    highlight: PlasmaComponents.Highlight {
    }

    function activate(index) {
        if (typeof contextMenu !== "undefined"
                && contextMenu.status !== PlasmaComponents.DialogStatus.Closed) {
            return
        }
        currentIndex = index
    }

    function clickCurrentItemButton(buttonName) {
        if (!currentItem) {
            return
        }
        var button = currentItem[buttonName]
        if (button && button.enabled) {
            button.clicked()
        }
    }

    function copyCurrentItemData(fieldName) {
        if (!currentItem) {
            return
        }
        var data = currentItem[fieldName]
        if (data) {
            plasmoid.nativeInterface.copyToClipboard(data)
        }
    }

    function showContextMenu(item, x, y) {
        if (typeof contextMenu === "undefined") {
            return
        }
        if (typeof contextMenu.init !== "undefined") {
            contextMenu.init(item)
        }
        contextMenu.open(x, y)
    }
}
