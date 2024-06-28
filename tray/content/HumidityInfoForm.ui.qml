/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick

ThermostatInfo {

    required property var humidityValuesModel
    property var humidityValues
    property int humidityAvg
    property int humidityDiff
    property bool isMore

    title: qsTr("Humidity")
    leftIcon: "images/drop.svg"
    topLabel: qsTr("Average: %1 %".arg(humidityAvg))
    bottomLeftLabel: isMore ? qsTr("%1 % more than Last Month".arg(humidityDiff)) :
                              qsTr("%1 % less than Last Month".arg(Math.abs(humidityDiff)))
    bottomLeftIcon: isMore ? "images/up" : "images/down"
}
