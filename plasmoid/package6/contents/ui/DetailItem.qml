import QtQuick 2.7
import QtQuick.Controls
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
            Layout.leftMargin: plasmoid.showSyncthingIcons ? Kirigami.Units.iconSizes.small * 1.1 : 0
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
            id: detailLabel
            Layout.leftMargin: Kirigami.Theme.defaultFont.pointSize * 0.9
            Layout.fillWidth: true
            ToolTip.delay: 1000
            text: detailValue
            elide: Text.ElideRight
            horizontalAlignment: Qt.AlignRight
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.RightButton
        onClicked: {
            const view = detailItem.ListView.view
            const coordinates = mapToItem(view, mouseX, mouseY)
            view.showContextMenu(detailItem, coordinates.x, coordinates.y)
        }
        onContainsMouseChanged: {
            const toolTip = detailLabel.ToolTip
            const text = detailTooltip
            text?.length > 0 && mouseArea.containsMouse ? toolTip.show(text, 5000) : toolTip.hide()
        }
    }
}
