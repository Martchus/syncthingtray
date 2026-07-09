import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RoundButton {
    Layout.preferredWidth: 36 * QuickUI.densityScaleIconButtons
    Layout.preferredHeight: 36 * QuickUI.densityScaleIconButtons
    display: AbstractButton.IconOnly
    hoverEnabled: true
    ToolTip.text: text
    ToolTip.visible: hovered || pressed
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    icon.width: 20 * QuickUI.densityScaleIconButtons
    icon.height: 20 * QuickUI.densityScaleIconButtons
    flat: QuickUI.densityScale < 1.0
    scale: Utils.winUI ? 0.8 : 1.0
}
