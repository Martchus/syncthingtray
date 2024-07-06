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

Page {
    id: root

    header: ToolBar {
        id: toolBar

        background: Rectangle {
            color: Constants.accentColor
        }

        RowLayout {
            anchors.fill: parent
            Image {
                id: qtLogo

                Layout.topMargin: 6
                Layout.bottomMargin: 6
                Layout.leftMargin: 38
                source: "images/logo"
                sourceSize.height: 50
            }

            Item {
                Layout.fillWidth: true
            }

            Image {
                id: themeTapButton

                source: "images/theme.svg"
                sourceSize.height: Constants.isSmallLayout ? 15 : 20
                sourceSize.width: Constants.isSmallLayout ? 15 : 20
                Layout.rightMargin: Constants.isSmallLayout ? 5 : 19
                visible: Constants.isSmallLayout || Constants.isMobileLayout

                TapHandler {
                    onTapped: AppSettings.isDarkTheme = !AppSettings.isDarkTheme
                }
            }
        }
    }

    background: Rectangle {
        color: Constants.accentColor
    }

    StackView {
        id: stackView

        anchors.left: sideMenu.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        initialItem: RoomsView {
            roomsList: roomsList
        }
    }

    SideBar {
        id: sideMenu

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 63
        height: parent.height

        menuOptions: menuItems
        roomsList: roomsList
    }

    BottomBar {
        id: bottomMenu

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: parent.width

        visible: false
        position: TabBar.Footer
        menuOptions: menuItems
        roomsList: roomsList
    }

    RoomsModel {
        id: roomsList
    }

    ListModel {
        id: menuItems

        ListElement {
            name: qsTr("Rooms")
            view: "RoomsView"
            iconSource: "home.svg"
        }
        ListElement {
            name: qsTr("Thermostat Control")
            view: "ThermostatView"
            iconSource: "thermostat.svg"
        }
        ListElement {
            name: qsTr("Stats")
            view: "StatisticsView"
            iconSource: "stats.svg"
        }
        ListElement {
            name: qsTr("Settings")
            view: "SettingsView"
            iconSource: "settings.svg"
        }
    }

    states: [
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout

            PropertyChanges {
                target: sideMenu
                visible: false
            }
            PropertyChanges {
                target: bottomMenu
                visible: true
            }
            PropertyChanges {
                target: stackView
                anchors.leftMargin: 0
            }
            PropertyChanges {
                target: qtLogo
                Layout.leftMargin: 19
                sourceSize.height: 25
                Layout.topMargin: 7
                Layout.bottomMargin: 7
            }
            PropertyChanges {
                target: toolBar
                height: 39
            }
            AnchorChanges {
                target: stackView
                anchors.left: parent.left
                anchors.bottom: bottomMenu.top
            }
        },
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout

            PropertyChanges {
                target: sideMenu
                visible: true
                anchors.topMargin: 63
            }
            PropertyChanges {
                target: bottomMenu
                visible: false
            }
            PropertyChanges {
                target: stackView
                anchors.leftMargin: 5
            }
            PropertyChanges {
                target: qtLogo
                Layout.leftMargin: 38
                sourceSize.height: 50
                Layout.topMargin: 6
                Layout.bottomMargin: 6
            }
            PropertyChanges {
                target: toolBar
                height: 56
            }
            AnchorChanges {
                target: stackView
                anchors.left: sideMenu.right
                anchors.bottom: parent.bottom
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout

            PropertyChanges {
                target: sideMenu
                visible: true
                anchors.topMargin: 24
            }
            PropertyChanges {
                target: bottomMenu
                visible: false
            }
            PropertyChanges {
                target: stackView
                anchors.leftMargin: 5
            }
            PropertyChanges {
                target: qtLogo
                Layout.leftMargin: 5
                sourceSize.height: 18
                Layout.topMargin: 5
                Layout.bottomMargin: 0
            }
            PropertyChanges {
                target: toolBar
                height: 24
            }
            AnchorChanges {
                target: stackView
                anchors.left: sideMenu.right
                anchors.bottom: parent.bottom
            }
        }
    ]
}
