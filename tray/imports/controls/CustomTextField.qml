import QtQuick
import QtQuick.Controls
import app

TextField {
    id: control

    width: 135
    height: 56

    text: "10"
    font.pixelSize: 24
    font.weight: 400
    font.family: "Titillium Web"
    color: Constants.primaryTextColor

    inputMethodHints: Qt.ImhDigitsOnly
    validator: IntValidator {
        bottom: 10
        top: 30
    }
    leftPadding: 15

    background: Rectangle {
        implicitHeight: control.height
        implicitWidth: control.width
        border.color: control.acceptableInput ? Constants.accentTextColor : "red"
        color: "transparent"
        radius: 8

        Label {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            text: "°C"
            font.pixelSize: 14
            color: "#898989"
        }

        Image {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 15
            source: "images/keyboard.svg"
        }
    }
}
