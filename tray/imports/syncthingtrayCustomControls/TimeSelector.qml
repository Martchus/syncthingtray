pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import syncthingtrayApp

Rectangle {
    id: root

    width: 1010
    height: 200
    radius: 12

    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Constants.backgroundColor
        }
        GradientStop {
            position: 1.0
            color: "transparent"
        }
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0

        Repeater {
            model: 24
            Item {
                id: column

                height: root.height
                width: internal.itemWidth

                Label {
                    id: hour

                    text: index
                    font.pixelSize: internal.hourSize
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    font.weight: 700
                    font.family: "Titillium Web"
                    color: "#2CDE85"
                    visible: index % 6 == 0 || index === 23
                }

                CustomRoundButton {
                    id: rec

                    height: internal.itemHeight
                    width: internal.itemWidth
                    contentColor: "#2CDE85"
                    text: ""
                    radius: 12
                    anchors.top: parent.top
                    anchors.topMargin: internal.itemTopMargin
                }

                Label {
                    id: bottomHour

                    text: index
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.weight: 600
                    font.family: "Titillium Web"
                    color: "#D9D9D9"
                    visible: internal.bottomHourVisibility
                }
            }
        }
    }

    QtObject {
        id: internal

        property int hourSize: 24
        property int itemWidth: 40
        property int itemHeight: 120
        property int itemTopMargin: 55
        property int columnHeight: 165
        property bool bottomHourVisibility: true
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: root
                width: 1010
                height: 200
            }
            PropertyChanges {
                target: internal
                hourSize: 24
                itemWidth: 40
                itemHeight: 120
                columnHeight: 165
                bottomHourVisibility: true
                itemTopMargin: 55
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: root
                width: 326
                height: 150
            }
            PropertyChanges {
                target: internal
                hourSize: 14
                itemWidth: 13
                itemHeight: 110
                columnHeight: 110
                bottomHourVisibility: false
                itemTopMargin: 32
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: root
                width: 388
                height: 150
            }
            PropertyChanges {
                target: internal
                hourSize: 14
                itemWidth: 16
                itemHeight: 110
                columnHeight: 110
                bottomHourVisibility: false
                itemTopMargin: 24
            }
        }
    ]
}
