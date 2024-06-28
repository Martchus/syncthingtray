import QtQuick

ThermostatSettingsForm {
    buttonGroup.onClicked: (button) => model.mode = button.text
}
