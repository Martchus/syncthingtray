import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    RoundButton {
        display: AbstractButton.IconOnly
        text: qsTr("Copy")
        ToolTip.text: text
        ToolTip.visible: hovered || pressed
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        icon.source: app.faUrlBase + "files-o"
        icon.width: 20
        icon.height: 20
        onClicked: app.copyText(edit.text)
    }
    RoundButton {
        display: AbstractButton.IconOnly
        text: qsTr("Paste")
        enabled: edit.enabled
        ToolTip.text: text
        ToolTip.visible: hovered || pressed
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        icon.source: app.faUrlBase + "clipboard"
        icon.width: 20
        icon.height: 20
        onClicked: edit.text = app.getClipboardText()
    }
    required property Item edit
}
