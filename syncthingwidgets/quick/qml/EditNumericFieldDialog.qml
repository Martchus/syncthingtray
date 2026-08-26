import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

CustomDialog {
    id: numberDlg
    title: modelData.label
    standardButtons: objectConfigPage.standardButtons
    contentItem: TextField {
        id: editedNumberValue
        focus: true
        text: modelData.value
        inputMethodHints: modelData.inputMethodHints ?? Qt.ImhNone
        validator: DoubleValidator {
            id: numberValidator
            locale: "en"
        }
        onAccepted: numberDlg.accept()
    }
    onAccepted: objectConfigPage.updateValue(modelData.index, modelData.key, Number.fromLocaleString(Qt.locale(numberValidator.locale), editedNumberValue.text))
    onRejected: editedNumberValue.text = objectConfigPage.configObject[modelData.key]
    onHelpRequested: helpButton.clicked()
    required property var helpButton
    property alias text: editedNumberValue.text
}
