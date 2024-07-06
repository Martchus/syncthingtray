/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import app
import controls

Pane {
    width: 1087
    height: 361

    padding: 0

    background: Rectangle {
        color: Constants.accentColor
        radius: 12
    }

    TemperatureSetterDesktopView {
        id: desktopView
    }

    TemperatureSetterMobileView {
        id: mobileView
        visible: false
    }

    TemperatureSetterSmallView {
        id: smallView
        visible: false
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: desktopView
                visible: true
            }
            PropertyChanges {
                target: mobileView
                visible: false
            }
            PropertyChanges {
                target: smallView
                visible: false
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: desktopView
                visible: false
            }
            PropertyChanges {
                target: mobileView
                visible: true
            }
            PropertyChanges {
                target: smallView
                visible: false
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: desktopView
                visible: false
            }
            PropertyChanges {
                target: mobileView
                visible: false
            }
            PropertyChanges {
                target: smallView
                visible: true
            }
        }
    ]
}
