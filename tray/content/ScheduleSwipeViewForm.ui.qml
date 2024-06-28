/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls

Item {
    id: root

    property alias model: repeater.model
    property alias swipeView: swipeView
    property alias currentRoomIndex: swipeView.currentIndex

    property bool isOneColumn: false

    ListView {
        id: roomSelector

        model: root.model
        width: root.width
        height: 28
        spacing: 26
        orientation: ListView.Horizontal
        delegate: Label {
            id: labelDelegate

            required property string name
            required property int index
            readonly property bool isActive: swipeView.currentIndex === index

            text: name
            font.pixelSize: 12
            font.family: "Titillium Web"
            font.weight: 400
            font.bold: isActive
            font.underline: isActive
            color: isActive ? "#2CDE85" : "#898989"

            MouseArea {
                anchors.fill: parent
                Connections {
                    function onClicked() {
                        swipeView.setCurrentIndex(labelDelegate.index)
                    }
                }
            }
        }
    }

    SwipeView {
        id: swipeView

        height: root.height - roomSelector.height + 13
        width: root.width

        anchors.top: roomSelector.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 7
        clip: true

        Repeater {
            id: repeater

            ScheduleScrollView {
                width: swipeView.width
                height: swipeView.height
            }
        }
    }
}
