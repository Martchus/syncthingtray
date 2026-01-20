import QtQuick
import QtQuick.Controls.Material

import Main

ToolButton {
    icon.width: QuickUI.iconSize
    icon.height: QuickUI.iconSize
    display: AbstractButton.IconOnly
    onPressAndHold: QuickUI.performHapticFeedback()
    ToolTip.text: text
    ToolTip.visible: hovered || pressed
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
