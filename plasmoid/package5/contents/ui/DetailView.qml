import QtQuick 2.7
import org.kde.plasma.components 2.0 as PlasmaComponents // for Menu and MenuItem

ListView {
    id: detailView
    property DetailItem contextMenuItem: null
    currentIndex: -1
    interactive: false
    height: contentHeight

    PlasmaComponents.Menu {
        id: contextMenu
        PlasmaComponents.MenuItem {
            text: qsTr('Copy value')
            icon: "edit-copy"
            onClicked: {
                var item = detailView.contextMenuItem
                if (item) {
                    plasmoid.nativeInterface.copyToClipboard(item.detailValue)
                }
            }
        }
    }

    function showContextMenu(item, x, y) {
        contextMenuItem = item
        contextMenu.open(x, y)
    }
}
