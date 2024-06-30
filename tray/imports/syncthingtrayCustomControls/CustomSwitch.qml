import QtQuick
import QtQuick.Controls
import syncthingtrayApp

Switch {
    id: control

    readonly property color gradientColor1: AppSettings.isDarkTheme ? "#FFFFFF" : "#898989"
    readonly property color gradientColor2: AppSettings.isDarkTheme ? "#002125" : "#FFFFFF"

    indicator: Rectangle {
        anchors.right: parent.right
        implicitWidth: internal.indicatorWidth
        implicitHeight: internal.indicatorHeight
        radius: 12
        border.color: "#898989"

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: !control.checked ? control.gradientColor1 : "#2CDE85"
            }
            GradientStop {
                position: 1.0
                color: !control.checked ? control.gradientColor2 : "#2CDE85"
            }
        }

        Rectangle {
            width: 1
            height: internal.separatorHeight
            anchors.verticalCenter: parent.verticalCenter
            x: control.checked ? internal.separatorStartPos : internal.separatorEndPos
            color: "#898989"
        }

        Rectangle {
            id: circle

            anchors.verticalCenter: parent.verticalCenter
            x: control.checked ? parent.width - width - 2 : 2
            width: internal.circleSize
            height: internal.circleSize
            radius: 10
            color: "#ffffff"
            border.color: "#898989"

            Behavior on x  {
                NumberAnimation {
                    duration: 250
                    easing.type: Easing.OutBack
                }
            }
        }
    }

    QtObject {
        id: internal

        property int circleSize: 20
        property int indicatorWidth: 55
        property int indicatorHeight: 24
        property int separatorHeight: 9
        property int separatorStartPos: 20
        property int separatorEndPos: 35
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: internal
                circleSize: 20
                indicatorWidth: 55
                indicatorHeight: 24
                separatorStartPos: 20
                separatorEndPos: 35
                separatorHeight: 9
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout || Constants.isMobileLayout
            PropertyChanges {
                target: internal
                circleSize: 12
                indicatorWidth: 35
                indicatorHeight: 15
                separatorStartPos: 12
                separatorEndPos: 22
                separatorHeight: 6
            }
        }
    ]
}
