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
    id: root

    property alias title: title.text
    property alias leftIcon: icon.source
    property alias rightIcon: icon2.source
    property alias topLabel: topLabel.text
    property alias bottomLeftIcon: bottomLeftIcon.source
    property alias bottomRightIcon: bottomRightIcon.source
    property alias bottomLeftLabel: bottomLeftLabel.text
    property alias bottomRightLabel: bottomRightLabel.text

    topPadding: 20
    leftPadding: 16
    rightPadding: 16
    bottomPadding: 14

    width: 350
    height: 182

    background: Rectangle {
        radius: 12
        color: Constants.accentColor
        border.color: Constants.isSmallLayout ? "#D9D9D9" : "transparent"
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: internal.columnSpacing

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: internal.rowSpacing

            Item {
                id: iconItem

                Layout.preferredWidth: icon.sourceSize.width
                Layout.preferredHeight: icon.sourceSize.height

                Image {
                    id: icon

                    source: "images/schedule.svg"
                }

                MultiEffect {
                    anchors.fill: icon
                    source: icon
                    colorization: 1
                    colorizationColor: Constants.iconColor
                }
            }

            Label {
                id: title

                text: qsTr("Room Schedule")
                font.weight: 600
                font.pixelSize: 24
                font.family: "Titillium Web"
                color: Constants.primaryTextColor
                Layout.fillWidth: true
            }

            Image {
                id: icon2
            }
        }

        Column {
            leftPadding: internal.infoLeftPadding
            spacing: internal.infoSpacing
            width: parent.width

            Label {
                id: topLabel

                text: qsTr("Afternoon: 21°C (12pm-5pm)")
                font.pixelSize: internal.infoFontSize
                font.bold: true
                font.family: "Titillium Web"
                color: Constants.primaryTextColor
            }

            RowLayout {
                spacing: 32

                Row {
                    spacing: 10

                    Image {
                        id: bottomLeftIcon
                    }

                    Label {
                        id: bottomLeftLabel

                        text: qsTr("Night: 25°C (10pm-6am)")
                        font.pixelSize: internal.infoFontSize
                        font.weight: 600
                        font.family: "Titillium Web"
                        color: Constants.primaryTextColor
                    }
                }

                Row {
                    spacing: 10

                    Image {
                        id: bottomRightIcon
                    }

                    Label {
                        id: bottomRightLabel

                        font.pixelSize: internal.infoFontSize
                        font.weight: 600
                        font.family: "Titillium Web"
                        color: Constants.primaryTextColor
                    }
                }
            }
        }
    }

    QtObject {
        id: internal

        property int rowSpacing: 24
        property int columnSpacing: 24
        property int infoSpacing: 8
        property int infoFontSize: 14
        property int infoLeftPadding: 14
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: internal
                rowSpacing: 24
                columnSpacing: 16
                infoSpacing: 16
                infoFontSize: 14
                infoLeftPadding: 0
            }
            PropertyChanges {
                target: icon
                sourceSize.width: 34
                sourceSize.height: 34
            }
            PropertyChanges {
                target: icon2
                sourceSize.width: 20
                sourceSize.height: 20
            }
            PropertyChanges {
                target: title
                font.pixelSize: 24
            }
            PropertyChanges {
                target: root
                topPadding: 20
                leftPadding: 16
                rightPadding: 16
                bottomPadding: 14
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: internal
                rowSpacing: 9
                columnSpacing: 11
                infoSpacing: 5
                infoFontSize: 12
                infoLeftPadding: 10
            }
            PropertyChanges {
                target: icon
                sourceSize.width: 20
                sourceSize.height: 20
            }
            PropertyChanges {
                target: icon2
                sourceSize.width: 15
                sourceSize.height: 15
            }
            PropertyChanges {
                target: title
                font.pixelSize: 14
            }
            PropertyChanges {
                target: root
                topPadding: 10
                leftPadding: 7
                rightPadding: 16
                bottomPadding: 8
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: internal
                rowSpacing: 5
                infoFontSize: 12
                columnSpacing: 3
                infoSpacing: 4
                infoLeftPadding: 13
            }
            PropertyChanges {
                target: icon
                sourceSize.width: 15
                sourceSize.height: 15
            }
            PropertyChanges {
                target: icon2
                sourceSize.width: 15
                sourceSize.height: 15
            }
            PropertyChanges {
                target: title
                font.pixelSize: 14
            }
            PropertyChanges {
                target: root
                topPadding: 4
                leftPadding: 5
                rightPadding: 10
                bottomPadding: 14
            }
        }
    ]
}
