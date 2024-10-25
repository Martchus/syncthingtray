import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    IconOnlyButton {
        text: qsTr("Copy")
        icon.source: app.faUrlBase + "files-o"
        onClicked: app.copyText(edit.text)
    }
    IconOnlyButton {
        text: qsTr("Paste")
        enabled: edit.enabled
        icon.source: app.faUrlBase + "clipboard"
        onClicked: edit.text = app.getClipboardText()
    }
    required property Item edit
}
