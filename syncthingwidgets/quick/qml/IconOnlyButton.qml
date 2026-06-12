import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RoundButton {
    Layout.preferredWidth: 36 * QuickUI.densityScale
    Layout.preferredHeight: 36 * QuickUI.densityScale
    display: AbstractButton.IconOnly
    hoverEnabled: true
    ToolTip.text: text
    ToolTip.visible: pressed
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    icon.width: 20 * QuickUI.densityScale
    icon.height: 20 * QuickUI.densityScale
}
