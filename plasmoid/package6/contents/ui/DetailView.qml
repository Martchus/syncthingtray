import QtQuick 2.7
import org.kde.plasma.extras 2.0 as PlasmaExtras

ListView {
    id: detailView
    property DetailItem contextMenuItem: null
    currentIndex: -1
    interactive: false

    onCountChanged: {
        var d = delegate.createObject(detailView, {detailName: "", detailValue: ""});
        height = count * d.height
        d.destroy()
    }

    PlasmaExtras.Menu {
        id: contextMenu
        PlasmaExtras.MenuItem {
            text: qsTr('Copy value')
            icon: "edit-copy"
            onClicked: {
                var item = detailView.contextMenuItem
                if (item) {
                    plasmoid.copyToClipboard(item.detailValue)
                }
            }
        }
    }

    function showContextMenu(item, x, y) {
        contextMenuItem = item
        contextMenu.open(x, y)
    }
}
