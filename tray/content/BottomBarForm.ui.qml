/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import syncthingtray

TabBar {
    id: root

    required property var roomsList
    property alias menuOptions: repeater.model

    contentHeight: 56

    background: Rectangle {
        color: Constants.accentColor
        border.color: Constants.accentTextColor
        radius: 12
    }

    Repeater {
        id: repeater

        delegate: TabButton {
            id: menuItem

            required property string name
            required property string view
            required property string iconSource
            readonly property bool active: Constants.currentView == menuItem.view

            background: Rectangle {
                color: "transparent"
            }

            width: menuItem.view == "SettingsView" ? 0 : undefined;
            height: menuItem.view == "SettingsView" ? 0 : undefined;

            icon.width: 24
            icon.height: 24
            icon.source: "images/" + menuItem.iconSource
            icon.color: menuItem.active ? "#2CDE85" : Constants.accentTextColor

            Connections {
                function onClicked() {
                    if (menuItem.view != "SettingsView"
                            && menuItem.view != Constants.currentView) {
                        stackView.replace(menuItem.view + ".qml", {
                                              "roomsList": roomsList
                                          }, StackView.Immediate)
                        Constants.currentView = menuItem.view
                    }
                }
            }
        }
    }
}
