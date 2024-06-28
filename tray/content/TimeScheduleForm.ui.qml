/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import syncthingtray
import syncthingtrayCustomControls

Pane {
    width: 1087
    height: 361

    topPadding: internal.topPadding

    background: Rectangle {
        color: Constants.accentColor
        radius: 12
    }

    RowLayout {
        width: parent.width
        Label {
            font.pixelSize: internal.fontSize
            font.weight: 600
            font.family: "Titillium Web"
            text: qsTr("Set Time :")
            Layout.fillWidth: true
            color: Constants.primaryTextColor
        }
        Button {
            text: "Select Date"
        }
    }

    TimeSelector {
        id: timeSelector
        anchors.top: parent.top
        anchors.topMargin: internal.topMargin
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Label {
        anchors.top: timeSelector.bottom
        anchors.left: timeSelector.left
        anchors.topMargin: internal.labelMargin
        color: Constants.accentTextColor
        text: qsTr("Click on a block to set the whole hour")
        font.pixelSize: 14
        font.weight: 400
        font.family: "Titillium Web"
    }

    QtObject {
        id: internal
        property int fontSize: 24
        property int topMargin: 67
        property int topPadding: 20
        property int labelMargin: 12
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: internal
                topMargin: 67
                topPadding: 20
                fontSize: 24
                labelMargin: 12
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: internal
                topMargin: 85
                topPadding: 16
                fontSize: 18
                labelMargin: 12
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: internal
                topMargin: 54
                topPadding: 10
                fontSize: 14
                labelMargin: -12
            }
        }
    ]
}
