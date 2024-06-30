import QtQuick
import QtQuick.Controls
import syncthingtrayApp

RoundButton {
    id: control

    property color contentColor: AppSettings.isDarkTheme ? "#D9D9D9" : "#898989"
    property color borderColor: AppSettings.isDarkTheme ? "#898989" : "#D9D9D9"

    width: 130
    height: 63
    text: qsTr("Button")

    font.pixelSize: 18
    font.weight: 600
    font.family: "Titillium Web"

    icon.width: 42
    icon.height: 42
    icon.color: (control.checked || control.pressed) && control.enabled ? "#FFFFFF" : control.contentColor

    palette.buttonText: control.pressed ? "#FFFFFF" : control.contentColor
    display: Constants.isSmallLayout ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
    checkable: true
    radius: 20

    background: Rectangle {
        color: control.checked || control.down ? "#2CDE85" : "transparent"
        border.color: control.enabled ? control.contentColor : control.borderColor
        radius: control.radius
    }
}
