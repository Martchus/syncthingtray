// Based on PlasmaComponents.ListItem from Plasma 5.44.0
// (Can't use PlasmaComponents.ListItem itself because creating a MouseArea filling
//  the entire entire item for detecting right-mouse-click is not possible due to binding
//  loop of width and height properties.)
import QtQuick 2.7
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    id: listItem
    property bool expanded: false
    default property alias content: paddingItem.data

    /**
     * If true makes the list item look as checked or pressed. It has to be set
     * from the code, it won't change by itself.
     */
    //plasma extension
    //always look pressed?
    property bool checked: false

    /**
     * If true the item will be a delegate for a section, so will look like a
     * "title" for the otems under it.
     */
    //is this to be used as section delegate?
    property bool sectionDelegate: false

    /**
     * type: bool
     * True if the separator between items is visible
     * default: true
     */
    property bool separatorVisible: true

    width: parent ? parent.width : childrenRect.width
    height: paddingItem.childrenRect.height + background.margins.top + background.margins.bottom

    implicitHeight: paddingItem.childrenRect.height + background.margins.top
                    + background.margins.bottom

    function activate(containsMouse) {
        view.activate(containsMouse ? index : -1)
    }

    PlasmaCore.FrameSvgItem {
        id: background
        imagePath: "widgets/listitem"
        prefix: (listItem.sectionDelegate ? "section" : (itemMouse.pressed
                                                         || listItem.checked) ? "pressed" : "normal")

        anchors.fill: parent
        visible: listItem.ListView.view ? listItem.ListView.view.highlight === null : true
        Behavior on opacity {
            NumberAnimation {
                duration: units.longDuration
            }
        }
    }
    PlasmaCore.SvgItem {
        svg: PlasmaCore.Svg {
            imagePath: "widgets/listitem"
        }
        elementId: "separator"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: naturalSize.height
        visible: separatorVisible && (listItem.sectionDelegate
                                      || (typeof (index) != "undefined"
                                          && index > 0 && !listItem.checked
                                          && !itemMouse.pressed))
    }

    MouseArea {
        id: itemMouse
        property bool changeBackgroundOnPress: !listItem.checked
                                               && !listItem.sectionDelegate
        anchors.fill: background
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: {
            switch (mouse.button) {
            case Qt.LeftButton:
                expanded = !expanded
                break
            case Qt.RightButton:
                var view = listItem.ListView.view
                var coordinates = mapToItem(view, mouseX, mouseY)
                view.showContextMenu(listItem, coordinates.x, coordinates.y)
                break
            }
        }

        onContainsMouseChanged: {
            listItem.activate(containsMouse)
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
