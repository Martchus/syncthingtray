import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import syncthingtray

Row {
    id: root

    required property bool isEnabled
    required property bool isHeating

    property alias tempValue: temp.text

    spacing: internal.rowSpacing

    Label {
        id: temp
        text: "24"
        font.pixelSize: internal.fontSize
        font.weight: 600
        font.family: "Titillium Web"
        color: internal.itemColor
    }

    Column {
        anchors.verticalCenter: temp.verticalCenter
        spacing: internal.columnSpacing

        Label {
            text: "oC"
            font.pixelSize: internal.smallFontSize
            font.weight: 600
            font.family: "Titillium Web"
            color: internal.itemColor
        }

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            height: internal.smallFontSize
            width: internal.smallFontSize / 2

            Image {
                id: icon
                sourceSize.height: internal.smallFontSize
                source: "images/thermometer.svg"
            }

            MultiEffect {
                anchors.fill: icon
                source: icon
                colorization: 1
                colorizationColor: root.isEnabled ? internal.thermometerColor : "#898989"
            }
        }
    }

    QtObject {
        id: internal
        property int rowSpacing: 7
        property int fontSize: 96
        property int columnSpacing: 20
        property int smallFontSize: 24

        readonly property color thermometerColor: root.isHeating ? "#D21313" : "#131AD2"
        readonly property color itemColor: root.isEnabled ? Constants.primaryTextColor : "#898989"
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: internal
                rowSpacing: 7
                fontSize: 96
                columnSpacing: 20
                smallFontSize: 24
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout || Constants.isSmallLayout
            PropertyChanges {
                target: internal
                rowSpacing: 0
                fontSize: 56
                columnSpacing: 12
                smallFontSize: 14
            }
        }
    ]
}
