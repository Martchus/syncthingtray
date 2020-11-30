import QtQuick 2.7
import QtQuick.Controls 2.15 as QQC2

ListView {
    id: detailView
    property DetailItem contextMenuItem: null
    currentIndex: -1
    interactive: false
    height: contentHeight

    QQC2.Menu {
        id: contextMenu
        QQC2.MenuItem {
            text: qsTr('Copy value')
            icon.name: "edit-copy"
            onTriggered: {
                var item = detailView.contextMenuItem
                if (item) {
                    plasmoid.nativeInterface.copyToClipboard(item.detailValue)
                }
            }
        }
    }

    function showContextMenu(item, x, y) {
        contextMenuItem = item
        contextMenu.popup()
    }
}
