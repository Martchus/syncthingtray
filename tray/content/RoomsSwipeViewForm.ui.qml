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
    property alias previousItem: previousItem
    property alias nextItem: nextItem

    required property int delegatePreferredHeight
    required property int delegatePreferredWidth

    ListView {
        id: roomSelector

        model: root.model
        orientation: ListView.Horizontal
        width: parent.width
        height: 28
        spacing: 26
        delegate: Label {
            id: labelDelegate

            required property string name
            required property int index

            text: name
            font.pixelSize: 12
            font.family: "Titillium Web"
            font.weight: 400
            font.bold: swipeView.currentIndex === index
            font.underline: swipeView.currentIndex === index
            color: swipeView.currentIndex === index ? "#2CDE85" : "#898989"

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

    Image {
        source: "images/arrow.svg"
        sourceSize.width: 35
        sourceSize.height: 35
        anchors.verticalCenter: swipeView.verticalCenter
        anchors.right: swipeView.left
        mirror: true

        TapHandler {
            id: previousItem
        }
    }

    Image {
        source: "images/arrow.svg"
        anchors.verticalCenter: swipeView.verticalCenter
        anchors.left: swipeView.right
        sourceSize.width: 35
        sourceSize.height: 35

        TapHandler {
            id: nextItem
        }
    }

    SwipeView {
        id: swipeView

        height: root.delegatePreferredHeight
        width: root.delegatePreferredWidth

        anchors.top: roomSelector.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 4

        spacing: 7
        clip: true

        Repeater {
            id: repeater

            RoomItem {
                id: roomItem

                height: root.delegatePreferredHeight
                width: root.delegatePreferredWidth
            }
        }
    }
}
