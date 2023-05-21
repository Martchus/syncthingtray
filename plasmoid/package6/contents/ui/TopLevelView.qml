import QtQuick 2.7
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

ListView {
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentHeight > height
    keyNavigationEnabled: true
    keyNavigationWraps: true
    currentIndex: -1

    highlightMoveDuration: 0
    highlightResizeDuration: 0
    highlight: PlasmaExtras.Highlight {
    }

    topMargin: Kirigami.Units.smallSpacing * 2
    bottomMargin: Kirigami.Units.smallSpacing * 2
    leftMargin: Kirigami.Units.smallSpacing * 2
    rightMargin: Kirigami.Units.smallSpacing * 2

    function effectiveWidth() {
        return width - leftMargin - rightMargin
    }

    function activate(index) {
        if (typeof contextMenu !== "undefined"
                && contextMenu.status !== PlasmaExtras.DialogStatus.Closed) {
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
            plasmoid.copyToClipboard(data)
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
