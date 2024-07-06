pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import app

Pane {
    id: root

    property alias targetTemp: thermostat.value
    property int currentTemp: 10

    layer.enabled: true
    layer.samples: 4

    background: Rectangle {
        color: "transparent"
    }

    CustomDial {
        id: thermostat

        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        width: internal.thermostatSize
        height: internal.thermostatSize
        handleWidth: internal.handleSize
        handleHeight: internal.handleSize
        shapeRadius: internal.shapeRadius
        shapeWidth: internal.shapeWidth

        from: 10
        to: 30
        stepSize: 1
    }

    TemperatureLabel {
        anchors.centerIn: thermostat

        isEnabled: root.enabled
        isHeating: thermostat.value > root.currentTemp
        tempValue: thermostat.value
    }

    PathView {
        model: 11

        delegate: Rectangle {
            required property int index
            readonly property color activeColor: root.enabled ? "#2CDE85" : "#B8F6D5"

            width: internal.dotSize
            height: internal.dotSize
            radius: internal.dotSize / 2
            color: thermostat.value - (index * 2) >= 10 ? activeColor : "#D9D9D9"
        }

        path: Path {
            PathAngleArc {
                centerX: thermostat.x + thermostat.width / 2
                centerY: thermostat.y + thermostat.height / 2
                radiusX: internal.dotsRadius
                radiusY: internal.dotsRadius
                startAngle: -240
                sweepAngle: 330
            }
        }
    }

    PathView {
        model: ["14°C", "20°C", "26°C"]

        delegate: Label {
            required property string modelData
            text: modelData
            color: AppSettings.isDarkTheme ? "#D9D9D9" : "#898989"
            font.pixelSize: internal.fontSize
            font.family: "Titillium Web"
        }

        path: Path {
            PathAngleArc {
                centerX: thermostat.x + thermostat.width / 2
                centerY: thermostat.y + thermostat.height / 2
                radiusX: internal.tempRadius
                radiusY: internal.tempRadius
                startAngle: -180
                sweepAngle: 270
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: thermostat.bottom
        spacing: internal.rowSpacing
        palette.highlight: "#2CDE85"
        palette.button: "#2CDE85"

        RoundButton {
            width: internal.buttonSize
            height: internal.buttonSize
            text: "-"

            onClicked: thermostat.decrease()
        }

        RoundButton {
            width: internal.buttonSize
            height: internal.buttonSize
            text: "+"

            onClicked: thermostat.increase()
        }
    }

    QtObject {
        id: internal

        property int thermostatSize: 300
        property int dotsRadius: 172
        property int shapeRadius: 100
        property int tempRadius: 228
        property int shapeWidth: 18
        property int handleSize: 32
        property int fontSize: 24
        property int dotSize: 8
        property int rowSpacing: 255
        property int buttonSize: 32
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: root
                width: 520
                height: 472
            }
            PropertyChanges {
                target: internal
                thermostatSize: 300
                dotsRadius: 172
                shapeRadius: 103
                tempRadius: 228
                shapeWidth: 18
                handleSize: 32
                fontSize: 24
                dotSize: 8
                rowSpacing: 190
                buttonSize: 32
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: root
                width: 327
                height: 300
            }
            PropertyChanges {
                target: internal
                thermostatSize: 175
                dotsRadius: 100
                shapeRadius: 60
                tempRadius: 128
                shapeWidth: 10
                handleSize: 18
                fontSize: 14
                dotSize: 6
                rowSpacing: 120
                buttonSize: 20
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: root
                width: 250
                height: 225
            }
            PropertyChanges {
                target: internal
                thermostatSize: 144
                dotsRadius: 82
                shapeRadius: 49
                tempRadius: 105
                shapeWidth: 8
                handleSize: 14
                fontSize: 10
                dotSize: 4
                rowSpacing: 110
                buttonSize: 20
            }
        }
    ]
}
