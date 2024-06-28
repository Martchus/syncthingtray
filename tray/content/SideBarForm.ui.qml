/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import syncthingtray
import syncthingtrayCustomControls

Column {
    id: root

    property alias menuOptions: repeater.model
    required property var roomsList

    leftPadding: internal.leftPadding
    spacing: internal.spacing

    Repeater {
        id: repeater
        model: menuOptions

        delegate: ItemDelegate {
            id: columnItem

            required property string name
            required property string view
            required property string iconSource

            readonly property bool active: Constants.currentView === columnItem.view

            width: column.width
            height: column.height

            background: Rectangle {
                color: active ? "#2CDE85" : "transparent"
                radius: Constants.isSmallLayout ? 4 : 12
                anchors.fill: parent
                opacity: Constants.isSmallLayout ? 0.3 : 0.1
            }

            Column {
                id: column
                padding: 0
                Item {
                    id: menuItem

                    width: internal.delegateWidth
                    height: internal.delegateHeight
                    visible: Constants.isSmallLayout == false || columnItem.view != "SettingsView"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: internal.leftMargin
                        anchors.rightMargin: internal.rightMargin
                        spacing: 24

                        Item {
                            Layout.preferredWidth: internal.iconWidth
                            Layout.preferredHeight: internal.iconWidth
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            Image {
                                id: icon

                                source: "images/" + columnItem.iconSource
                                sourceSize.width: internal.iconWidth
                            }

                            MultiEffect {
                                anchors.fill: icon
                                source: icon
                                colorization: 1
                                colorizationColor: Constants.iconColor
                            }
                        }

                        Label {
                            id: menuItemName
                            text: name
                            font.family: "Titillium Web"
                            font.pixelSize: 18
                            font.weight: 600
                            Layout.fillWidth: true
                            visible: internal.isNameVisible
                            color: Constants.primaryTextColor
                        }

                        Image {
                            source: "images/arrow.svg"
                            visible: !columnItem.active
                                     && internal.isNameVisible
                        }
                    }
                }

                ListView {
                    model: [qsTr("Dark/Light")]
                    width: internal.delegateWidth
                    height: 44
                    visible: (Constants.isBigDesktopLayout
                              || Constants.isSmallDesktopLayout)
                             && columnItem.active
                             && columnItem.view == "SettingsView"
                    delegate: ItemDelegate {
                        id: item1
                        width: internal.delegateWidth
                        height: 44

                        RowLayout {
                            id: row

                            width: Constants.isBigDesktopLayout ? 190 : 18
                            spacing: 6
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter

                            Image {
                                source: "images/theme.svg"
                            }

                            Label {
                                text: modelData
                                Layout.fillWidth: true
                                color: Constants.primaryTextColor
                                visible: internal.isNameVisible
                            }
                        }

                        Connections {
                            function onClicked() {
                                AppSettings.isDarkTheme = !AppSettings.isDarkTheme
                            }
                        }
                    }
                }
            }

            Connections {
                function onClicked() {
                    if (columnItem.view != "SettingsView"
                            && columnItem.view != Constants.currentView) {
                        stackView.replace(columnItem.view + ".qml", {
                                              "roomsList": roomsList
                                          }, StackView.Immediate)
                    }
                    Constants.currentView = columnItem.view
                }
            }
        }
    }

    states: [
        State {
            name: "bigDesktop"
            when: Constants.isBigDesktopLayout
            PropertyChanges {
                target: internal
                delegateWidth: 290
                delegateHeight: 60
                iconWidth: 34
                isNameVisible: true
                leftMargin: 31
                rightMargin: 13
                leftPadding: 5
                spacing: 5
            }
        },
        State {
            name: "smallDesktop"
            when: Constants.isSmallDesktopLayout
            PropertyChanges {
                target: internal
                delegateWidth: 56
                delegateHeight: 56
                iconWidth: 34
                isNameVisible: false
                leftMargin: 0
                rightMargin: 0
                leftPadding: 5
                spacing: 5
            }
        },
        State {
            name: "small"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: internal
                delegateWidth: 24
                delegateHeight: 24
                iconWidth: 24
                isNameVisible: false
                leftMargin: 0
                rightMargin: 0
                leftPadding: 6
                spacing: 12
            }
        }
    ]

    QtObject {
        id: internal

        property int delegateWidth: 290
        property int delegateHeight: 60
        property int iconWidth: 34
        property bool isNameVisible: true
        property int leftMargin: 5
        property int rightMargin: 13
        property int leftPadding: 5
        property int spacing: 5
    }
}
