/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import app
import controls

Pane {
    width: 329
    height: 527

    topPadding: 16
    bottomPadding: 19

    background: Rectangle {
        color: Constants.accentColor
        radius: 12
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15
        Row {
            spacing: 10

            Item {
                width: 24
                height: 24
                Image {
                    id: icon
                    source: "images/temperature.svg"
                    sourceSize.width: 24
                    sourceSize.height: 24
                }
                MultiEffect {
                    anchors.fill: icon
                    source: icon
                    colorization: 1
                    colorizationColor: Constants.iconColor
                }
            }

            Label {
                font.pixelSize: 18
                font.weight: 600
                font.family: "Titillium Web"
                text: qsTr("Set Temperature :")
                color: Constants.primaryTextColor
            }
        }

        CustomTextField {
            width: 102
            height: 40
            font.pixelSize: 14
            text: slider.value
            onAccepted: slider.value = +text
        }

        Item {
            id: item1
            Layout.fillWidth: true
            Layout.fillHeight: true
            CustomSlider {
                id: slider
                width: parent.width
                anchors.bottom: parent.bottom
            }
        }

        Column {
            Label {
                font.pixelSize: 18
                font.weight: 600
                font.family: "Titillium Web"
                text: qsTr("Mode")
                color: Constants.primaryTextColor
            }

            Repeater {
                model: [qsTr("Heating"), qsTr("Cooling"), qsTr("Auto")]
                CustomRadioButton {
                    text: modelData
                    font.pixelSize: 14
                    indicatorSize: 14
                }
            }
        }

        ColumnLayout {
            spacing: 12
            Label {
                font.pixelSize: 18
                font.weight: 600
                font.family: "Titillium Web"
                text: qsTr("Repeat")
                color: Constants.primaryTextColor
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Layout.fillWidth: true
                spacing: 24
                Repeater {
                    id: repeater
                    model: [qsTr("MO"), qsTr("TU"), qsTr("WE"), qsTr(
                            "TH"), qsTr("FR"), qsTr("SA"), qsTr("SU")]

                    Label {
                        property bool checked: false

                        text: modelData
                        Layout.fillWidth: true
                        height: 21
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        color: checked ? "#2CDE85" : Constants.primaryTextColor
                        TapHandler {
                            onTapped: parent.checked = !parent.checked
                        }
                    }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [qsTr("Cancel"), qsTr("Save")]

                CustomRoundButton {
                    height: 45
                    Layout.fillWidth: true
                    text: modelData
                    radius: 12
                    contentColor: "#2CDE85"
                    checkable: false
                    font.pixelSize: 14
                }
            }
        }
    }
}
