import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

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
            id: icon
            Layout.leftMargin: units.iconSizes.small * 1.1
            Layout.preferredHeight: units.iconSizes.small
            Layout.preferredWidth: height
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            source: detailIcon
            opacity: 0.8
        }
        PlasmaComponents.Label {
            Layout.preferredWidth: 100
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: detailName
            font.weight: Font.DemiBold
            verticalAlignment: Qt.AlignVCenter
        }
        PlasmaComponents.Label {
            Layout.leftMargin: theme.defaultFont.pointSize * 0.9
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: detailValue
            elide: Text.ElideRight
            horizontalAlignment: Qt.AlignRight
            verticalAlignment: Qt.AlignVCenter
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
