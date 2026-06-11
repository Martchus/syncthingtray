// Based on PlasmaComponents.ListItem from Plasma 5.44.0
import QtQuick 2.7
import org.kde.ksvg 1.0 as KSvg
import org.kde.kirigami 2.20 as Kirigami

Item {
    id: listItem
    default property alias content: paddingItem.data
    property bool expanded: false

    width: parent ? parent.width : childrenRect.width
    height: paddingItem.childrenRect.height + background.margins.top + background.margins.bottom
    implicitHeight: paddingItem.childrenRect.height + background.margins.top
                    + background.margins.bottom

    function activate() {
        view.activate(index)
    }

    KSvg.FrameSvgItem {
        id: background
        imagePath: "widgets/listitem"
        prefix: "normal"
        anchors.fill: parent
        visible: listItem.ListView.view ? listItem.ListView.view.highlight === null : true
        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
            }
        }
    }

    MouseArea {
        id: itemMouse
        property bool changeBackgroundOnPress: !listItem.checked
        anchors.fill: background
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onEntered: listItem.activate()
        onClicked: function(mouse) {
            switch (mouse.button) {
            case Qt.LeftButton:
                expanded = !expanded
                break
            case Qt.RightButton:
                const view = listItem.ListView.view
                const coordinates = mapToItem(view, mouseX, mouseY)
                view.showContextMenu(listItem, coordinates.x, coordinates.y)
                break
            }
        }

        Item {
            id: paddingItem
            anchors {
                fill: parent
                leftMargin: background.margins.left
                topMargin: background.margins.top
                rightMargin: background.margins.right
                bottomMargin: background.margins.bottom
            }
        }
    }

    Accessible.role: Accessible.ListItem
}
