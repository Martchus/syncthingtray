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
import syncthingtray
import syncthingtrayCustomControls

Pane {
    width: 1087
    height: 427

    topPadding: 30
    bottomPadding: 24
    leftPadding: 16
    rightPadding: 36

    background: Rectangle {
        color: Constants.accentColor
        radius: 12
    }

    Row {
        id: row1
        width: parent.width
        height: 70
        spacing: 80

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 24

            Item {
                width: internal.iconSize
                height: internal.iconSize

                Image {
                    id: icon
                    source: "images/temperature.svg"
                    sourceSize.width: internal.iconSize
                    sourceSize.height: internal.iconSize
                }

                MultiEffect {
                    anchors.fill: icon
                    source: icon
                    colorization: 1
                    colorizationColor: Constants.iconColor
                }
            }

            Label {
                font.pixelSize: internal.fontSize
                font.weight: 600
                font.family: "Titillium Web"
                text: qsTr("Set Temperature :")
                color: Constants.primaryTextColor
            }
        }

        CustomTextField {
            anchors.verticalCenter: parent.verticalCenter
            text: slider.value
            onAccepted: slider.value = +text
        }

        CustomSlider {
            id: slider
            anchors.bottom: parent.bottom
        }
    }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 50
        Row {
            id: row
            spacing: 70
            Label {
                font.pixelSize: 24
                font.weight: 600
                font.family: "Titillium Web"
                text: qsTr("Mode")
                anchors.verticalCenter: parent.verticalCenter
                color: Constants.primaryTextColor
            }

            Repeater {
                model: [qsTr("Heating"), qsTr("Cooling"), qsTr("Auto")]
                CustomRadioButton {
                    text: modelData
                    indicatorSize: 20
                }
            }
        }

        Row {
            spacing: 24
            Label {
                font.pixelSize: 24
                font.weight: 600
                font.family: "Titillium Web"
                text: qsTr("Repeat")
                anchors.verticalCenter: parent.verticalCenter
                color: Constants.primaryTextColor
            }

            Repeater {
                model: [qsTr("Mon"), qsTr("Tue"), qsTr("Wed"), qsTr(
                        "Thu"), qsTr("Fri"), qsTr("Sat"), qsTr("Sun")]

                CustomRoundButton {
                    text: modelData
                    width: 90
                    height: 50
                    radius: 12
                    font.pixelSize: 24
                }
            }
        }
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        spacing: 24

        Repeater {
            model: [qsTr("Cancel"), qsTr("Save")]
            CustomRoundButton {
                width: 120
                height: 48
                text: modelData
                radius: 12
                contentColor: "#2CDE85"
                checkable: false
                font.pixelSize: 14
            }
        }
    }

    QtObject {
        id: internal
        property int fontSize: 24
        property int topMargin: 67
        property int iconSize: 34
    }
}
