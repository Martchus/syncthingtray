import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

RowLayout {
    IconOnlyButton {
        text: qsTr("Copy")
        icon.source: QuickUI.faUrlBase + "files-o"
        onClicked: SyncthingModels.copyText(edit[textProperty])
    }
    IconOnlyButton {
        text: qsTr("Paste")
        enabled: edit.enabled
        icon.source: QuickUI.faUrlBase + "clipboard"
        onClicked: edit[textProperty] = SyncthingModels.getClipboardText()
    }
    IconOnlyButton {
        text: qsTr("Clear")
        icon.source: QuickUI.faUrlBase + "eraser"
        onClicked: edit[textProperty] = ""
    }
    required property Item edit
    property string textProperty: "text"
}
