import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

CustomDialog {
    id: dlg
    title: modelData.label
    standardButtons: objectConfigPage.standardButtons
    width: parent.width - 20
    contentItem: RowLayout {
        TextField {
            id: textField
            Layout.fillWidth: true
            text: modelData.value
            inputMethodHints: modelData.inputMethodHints ?? Qt.ImhNone
            enabled: modelData?.enabled ?? true
            onAccepted: dlg.accept()
        }
        IconOnlyButton {
            visible: textField.enabled && (modelData.random === true || modelData.key === "id")
            text: qsTr("Make random ID")
            icon.source: App.faUrlBase + "hashtag"
            onClicked: objectConfigPage.requestRandomValue((value) => textField.text = value)
        }
        CopyPasteButtons {
            edit: textField
        }
    }
    onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, textField.text)
    onRejected: textField.text = objectConfigPage.configObject[modelData.key]
    onHelpRequested: dlg.helpButton.clicked()
    required property var helpButton
    property alias text: textField.text
}
