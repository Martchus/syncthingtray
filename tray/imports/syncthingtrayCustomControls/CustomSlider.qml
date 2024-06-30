import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import syncthingtrayApp

Slider {
    id: control

    from: 10
    value: 10
    to: 30

    snapMode: Slider.SnapAlways
    stepSize: 1

    readonly property color mainColor: AppSettings.isDarkTheme ? "#2CDE85" : "#00414A"
    readonly property color backgroundColor: AppSettings.isDarkTheme ? "#D9D9D9" : "#2CDE85"

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 250
        implicitHeight: 4
        width: control.availableWidth
        height: implicitHeight
        radius: 2
        color: control.backgroundColor

        Row {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 2
            spacing: 24
            Repeater {
                model: control.background.implicitWidth / parent.spacing
                Rectangle {
                    width: 2
                    height: 2
                    color: "#67AC94"
                }
            }
        }

        Rectangle {
            x: control.handle.x - control.leftPadding - width / 4
            y: control.handle.y + control.topPadding - height / 2 - height / 4
            implicitWidth: 40
            implicitHeight: 40
            radius: 20
            color: "#2CDE85"
            opacity: 0.1
            visible: control.hovered || control.pressed
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: control.mainColor
    }

    ToolTip {
        id: toolTip

        parent: control.handle
        visible: control.pressed
        width: 32
        height: 34

        background: Item {
            width: icon.width
            height: icon.height

            Image {
                id: icon
                source: "images/tooltip.svg"
            }
            MultiEffect {
                anchors.fill: icon
                source: icon
                colorization: 1
                colorizationColor: control.mainColor
            }
        }

        contentItem: Text {
            text: control.value + "°C"
            horizontalAlignment: Text.AlignHCenter
            color: "#FFFFFF"
        }
    }
}
