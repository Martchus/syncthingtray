/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick

ThermostatInfo {

    property real maxTempValue: 26
    property real minTempValue: 18
    property real avgTempValue: 22

    title: qsTr("Temperature")
    leftIcon: "images/temperature"
    topLabel: qsTr("Average: %1°C".arg(avgTempValue))
    bottomLeftLabel: qsTr("Minimum: %1°C".arg(minTempValue))
    bottomLeftIcon: "images/minTemp.svg"
    bottomRightLabel: qsTr("Maximum: %1°C".arg(maxTempValue))
    bottomRightIcon: "images/maxTemp.svg"
}
