import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

Item {
    id: detailItem

    property string detailName: name ? name : ""
    property string detailValue: detail ? detail : ""

    width: detailRow.implicitWidth
    height: detailRow.implicitHeight

    RowLayout {
        id: detailRow
        width: parent.width

        Kirigami.Icon {
            source: detailIcon
            Layout.leftMargin: Kirigami.Units.iconSizes.small * 1.1
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
            opacity: 0.8
        }
        PlasmaComponents3.Label {
            Layout.preferredWidth: 100
            text: detailName
            font.weight: Font.DemiBold
        }
        PlasmaComponents3.Label {
            Layout.leftMargin: Kirigami.Theme.defaultFont.pointSize * 0.9
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
