import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    IconOnlyButton {
        text: qsTr("Copy")
        icon.source: App.faUrlBase + "files-o"
        onClicked: App.copyText(edit[textProperty])
    }
    IconOnlyButton {
        text: qsTr("Paste")
        enabled: edit.enabled
        icon.source: App.faUrlBase + "clipboard"
        onClicked: edit[textProperty] = App.getClipboardText()
    }
    required property Item edit
    property string textProperty: "text"
}
