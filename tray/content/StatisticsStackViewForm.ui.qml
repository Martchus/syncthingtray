/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property alias model: repeater.model

    property bool isOneColumn: false
    property int currentRoomIndex

    StackLayout {
        anchors.fill: parent
        currentIndex: root.currentRoomIndex

        Repeater {
            id: repeater

            StatisticsScrollView {
                required property int index

                width: root.width
                height: root.height

                isOneColumn: root.isOneColumn
            }
        }
    }
}
