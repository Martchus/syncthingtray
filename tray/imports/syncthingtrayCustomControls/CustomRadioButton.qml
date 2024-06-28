import QtQuick
import QtQuick.Controls
import syncthingtray

RadioButton {
    id: control

    readonly property color indicatorColor: AppSettings.isDarkTheme ? "#D9D9D9" : "#202020"
    property var indicatorSize

    text: qsTr("Heating")
    font.family: "Titillium Web"
    font.pixelSize: 18
    font.weight: 400

    spacing: 11
    topPadding: 6
    bottomPadding: 6

    contentItem: Text {
        color: control.indicatorColor
        font: control.font
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Rectangle {
        border.color: control.indicatorColor
        color: "transparent"
        implicitHeight: control.indicatorSize
        implicitWidth: control.indicatorSize
        radius: control.indicatorSize / 2
        x: control.leftPadding
        y: parent.height / 2 - height / 2

        Rectangle {
            color: control.indicatorColor
            implicitHeight: parent.implicitHeight / 2
            implicitWidth: parent.implicitWidth / 2
            radius: parent.radius / 2
            visible: control.checked
            x: parent.implicitWidth / 4
            y: parent.implicitHeight / 4
        }
    }
}
