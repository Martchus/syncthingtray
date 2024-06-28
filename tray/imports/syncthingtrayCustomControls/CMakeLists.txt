qt_add_library(${META_PROJECT_VARNAME}CustomControls STATIC)
qt_add_qml_module(${META_PROJECT_VARNAME}CustomControls
    URI "${META_PROJECT_VARNAME}CustomControls"
    VERSION 1.0
    QML_FILES
        CustomComboBox.qml
        CustomDial.qml
        CustomRadioButton.qml
        CustomRoundButton.qml
        CustomSlider.qml
        CustomSwitch.qml
        CustomTextField.qml
        StatisticsChart.qml
        TemperatureLabel.qml
        ThermostatControl.qml
        TimeSelector.qml
    RESOURCES
        images/keyboard.svg
        images/thermometer.svg
        images/tooltip.svg
)
