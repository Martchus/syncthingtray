import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

Item {
    id: detailItem

    property string detailName: name ? name : ""
    property string detailValue: detail ? detail : ""

    width: detailRow.implicitWidth
    height: detailRow.implicitHeight

    RowLayout {
        id: detailRow
        width: parent.width

        PlasmaCore.IconItem {
            source: detailIcon
            Layout.leftMargin: units.iconSizes.small * 1.1
            Layout.preferredWidth: units.iconSizes.small
            Layout.preferredHeight: units.iconSizes.small
            opacity: 0.8
        }
        PlasmaComponents3.Label {
            Layout.preferredWidth: 100
            text: detailName
            font.weight: Font.DemiBold
        }
        PlasmaComponents3.Label {
            Layout.leftMargin: theme.defaultFont.pointSize * 0.9
            Layout.fillWidth: true
            text: detailValue
            elide: Text.ElideRight
            horizontalAlignment: Qt.AlignRight
        }
    }

    MouseArea {
        acceptedButtons: Qt.RightButton
        anchors.fill: parent
        onClicked: {
            var view = detailItem.ListView.view
            var coordinates = mapToItem(view, mouseX, mouseY)
            view.showContextMenu(detailItem, coordinates.x, coordinates.y)
        }
    }
}
