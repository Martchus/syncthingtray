import QtQuick
import QtQuick.Controls.Material

import Main

ToolButton {
    icon.width: App.iconSize
    icon.height: App.iconSize
    display: AbstractButton.IconOnly
    onPressAndHold: App.performHapticFeedback()
    ToolTip.text: text
    ToolTip.visible: hovered || pressed
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
