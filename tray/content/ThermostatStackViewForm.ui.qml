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

    property bool isOneColumn: false
    property var model
    property int currentRoomIndex

    StackLayout {
        anchors.fill: parent
        currentIndex: currentRoomIndex

        Repeater {
            id: repeater
            model: root.model

            ThermostatScrollView {
                id: delegate

                required property string name
                required property var model

                width: root.width
                height: root.height
                isOneColumn: root.isOneColumn

                roomName: name
                isActive: model.active
            }
        }
    }
}
