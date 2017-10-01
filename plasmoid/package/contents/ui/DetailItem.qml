import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents

RowLayout {
    id: detailItem
    property string detailName: name ? name : ""
    property string detailValue: detail ? detail : ""

    PlasmaComponents.Label {
        Layout.preferredWidth: 100
        Layout.leftMargin: units.iconSizes.smallMedium
        text: detailName
        font.pointSize: theme.defaultFont.pointSize * 0.8
        elide: Text.ElideRight
    }
    PlasmaComponents.Label {
        Layout.fillWidth: true
        text: detailValue
        font.pointSize: theme.defaultFont.pointSize * 0.8
        elide: Text.ElideRight
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            var view = detailItem.ListView.view
            var coordinates = mapToItem(view, mouseX, mouseY)
            view.showContextMenu(detailItem, coordinates.x, coordinates.y)
        }
    }
}
