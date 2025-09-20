import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

RoundButton {
    Layout.preferredWidth: 36
    Layout.preferredHeight: 36
    display: AbstractButton.IconOnly
    hoverEnabled: true
    ToolTip.text: text
    ToolTip.visible: pressed
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    icon.width: 20
    icon.height: 20
}
