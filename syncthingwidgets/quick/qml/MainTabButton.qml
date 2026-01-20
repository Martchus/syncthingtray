import QtQuick
import QtQuick.Controls.Material

import Main

TabButton {
    display: parent.width > 400 ? AbstractButton.TextUnderIcon : AbstractButton.IconOnly
    font.pointSize: 7
    icon.source: QuickUI.faUrlBase + iconName
    icon.width: QuickUI.iconSize
    icon.height: QuickUI.iconSize
    onClicked: pageStack.setCurrentIndex(tabIndex)
    onPressAndHold: QuickUI.performHapticFeedback()
    ToolTip.visible: hovered || pressed
    ToolTip.text: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    required property string iconName
    required property int tabIndex
}
