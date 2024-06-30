import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import syncthingtrayApp

Dial {
    id: control

    property alias handleHeight: handleItem.height
    property alias handleWidth: handleItem.width
    property int shapeRadius
    property int shapeWidth

    background: Rectangle {
        border.color: "#2CDE85"
        height: width
        implicitHeight: 140
        implicitWidth: 140
        opacity: control.enabled ? 1 : 0.3
        radius: width / 2
        width: Math.max(64, Math.min(control.width, control.height))
        x: control.width / 2 - width / 2
        y: control.height / 2 - height / 2

        gradient: Gradient {
            GradientStop {
                color: AppSettings.isDarkTheme ? "#558276" : "#EFFCF6"
                position: 0.0
            }
            GradientStop {
                color: Constants.accentColor
                position: 1.0
            }
        }
    }
    handle: Rectangle {
        id: handleItem

        antialiasing: true
        border.color: "#2CDE85"
        border.width: 4
        color: "#FFFFFF"
        height: 30
        opacity: control.enabled ? 1 : 0.3
        radius: width / 2
        width: 30
        x: control.background.x + control.background.width / 2 - width / 2
        y: control.background.y + control.background.height / 2 - height / 2
        z: shape.z + 1

        transform: [
            Translate {
                y: -Math.min(control.background.width, control.background.height) * 0.4 + handleItem.height / 2
            },
            Rotation {
                angle: control.angle
                origin.x: handleItem.width / 2
                origin.y: handleItem.height / 2
            }
        ]
    }

    Shape {
        id: shape

        antialiasing: true

        ShapePath {
            fillColor: "transparent"
            strokeColor: control.enabled ? "#2CDE85" : AppSettings.isDarkTheme ? "#125F46" : "#B8F6D5"
            strokeWidth: control.shapeWidth

            PathAngleArc {
                centerX: control.width / 2
                centerY: control.height / 2
                radiusX: control.shapeRadius
                radiusY: control.shapeRadius
                startAngle: -242
                sweepAngle: control.angle + 140
            }
        }
    }
}
