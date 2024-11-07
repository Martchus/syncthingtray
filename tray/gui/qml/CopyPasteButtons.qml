import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

RowLayout {
    IconOnlyButton {
        text: qsTr("Copy")
        icon.source: App.faUrlBase + "files-o"
        onClicked: App.copyText(edit.text)
    }
    IconOnlyButton {
        text: qsTr("Paste")
        enabled: edit.enabled
        icon.source: App.faUrlBase + "clipboard"
        onClicked: edit.text = App.getClipboardText()
    }
    required property Item edit
}
