import QtQuick
import QtQuick.Controls.Material

import Main

TabButton {
    display: parent.width > 400 ? AbstractButton.TextUnderIcon : AbstractButton.IconOnly
    font.pointSize: 7
    icon.source: App.faUrlBase + iconName
    icon.width: App.iconSize
    icon.height: App.iconSize
    onClicked: pageStack.setCurrentIndex(tabIndex)
    onPressAndHold: App.performHapticFeedback()
    ToolTip.visible: hovered || pressed
    ToolTip.text: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    required property string iconName
    required property int tabIndex
}
