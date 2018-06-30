import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents

Item {
    id: detailItem

    property string detailName: name ? name : ""
    property string detailValue: detail ? detail : ""

    width: detailRow.implicitWidth
    height: detailRow.implicitHeight

    RowLayout {
        id: detailRow
        spacing: theme.defaultFont.pointSize * 0.8

        PlasmaComponents.Label {
            Layout.preferredWidth: 100
            Layout.leftMargin: units.iconSizes.smallMedium
            text: detailName + ":"
            font.pointSize: theme.defaultFont.pointSize * 0.8
            font.weight: Font.DemiBold
            horizontalAlignment: Qt.AlignRight
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            text: detailValue
            font.pointSize: theme.defaultFont.pointSize * 0.8
            elide: Text.ElideRight
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
