/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: scrollView

    clip: true
    contentWidth: availableWidth

    property alias model: repeater.model
    property alias columns: gridLayout.columns
    property alias gridWidth: gridLayout.width
    property alias gridHeight: gridLayout.height

    required property int delegatePreferredHeight
    required property int delegatePreferredWidth

    GridLayout {
        id: gridLayout

        columnSpacing: 24
        rowSpacing: 25

        Repeater {
            id: repeater

            RoomItem {
                id: roomItem

                Layout.preferredHeight: scrollView.delegatePreferredHeight
                Layout.preferredWidth: scrollView.delegatePreferredWidth
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
