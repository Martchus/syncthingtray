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
import syncthingtrayCustomControls
import syncthingtrayApp

Pane {
    id: root

    property bool isSmallLayout: false

    property alias isEnabled: toggle.checked
    property alias toggle: toggle

    required property string name
    required property string floor
    required property string iconName
    required property int temp
    required property int humidity
    required property int energy
    required property bool active
    required property var model

    width: internal.width
    height: internal.height

    topPadding: 12
    leftPadding: internal.leftPadding
    bottomPadding: 0
    rightPadding: 16

    background: Rectangle {
        radius: 12
        color: Constants.accentColor
    }

    RowLayout {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: internal.switchMargin
        spacing: 16

        Item {
            Layout.preferredWidth: internal.iconSize
            Layout.preferredHeight: internal.iconSize
            Layout.alignment: Qt.AlignTop

            Image {
                id: icon

                source: "images/" + root.iconName
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

        Column {
            id: title

            Layout.fillWidth: true
            spacing: internal.titleSpacing

            Label {
                text: root.name
                font.pixelSize: !root.isSmallLayout ? 24 : 18
                font.weight: 600
                font.family: "Titillium Web"
                color: Constants.primaryTextColor
            }

            Label {
                text: root.floor
                font.pixelSize: 10
                font.weight: 400
                font.family: "Titillium Web"
                color: Constants.accentTextColor
            }
        }

        CustomSwitch {
            id: toggle
            checked: root.active
            onCheckedChanged: model.active = toggle.checked
        }
    }

    Column {
        id: column

        spacing: internal.spacing
        anchors.left: parent.left
        anchors.leftMargin: internal.columnMargin
        anchors.top: header.bottom
        anchors.topMargin: !root.isSmallLayout ? 10 : 8

        Repeater {
            model: [qsTr("Humidity: %1%".arg(humidity)), qsTr(
                    "Energy Usage: %1 kWh".arg(energy))]

            Label {
                text: modelData
                font.pixelSize: !root.isSmallLayout ? 14 : 12
                font.weight: 400
                font.family: "Titillium Web"
                color: toggle.checked ? Constants.primaryTextColor : "#898989"
            }
        }
    }

    TemperatureLabel {
        id: temp

        anchors.verticalCenter: column.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: internal.rightMargin
        isEnabled: root.isEnabled
        isHeating: root.model.thermostatTemp > root.model.temp
        tempValue: root.temp
    }

    Rectangle {
        id: separator

        anchors.bottom: menu.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#DCDCDC"
    }

    ListModel {
        id: roomOptions
        ListElement {
            name: "Cool"
        }
        ListElement {
            name: "Heat"
        }
        ListElement {
            name: "Dry"
        }
        ListElement {
            name: "Fan"
        }
        ListElement {
            name: "Eco"
        }
        ListElement {
            name: "Auto"
        }
    }

    RowLayout {
        id: menu

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: !root.isSmallLayout ? 24 : 0

        Repeater {
            model: roomOptions

            RoomOption {
                id: roomOption

                Layout.fillWidth: true
                Layout.fillHeight: true

                isEnabled: root.isEnabled
                isSmallLayout: root.isSmallLayout
                isActive: root.model.mode === roomOption.name

                Connections {
                    function onClicked() {
                        root.model.mode = roomOption.name
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 30
        }

        Item {
            Layout.preferredWidth: !root.isSmallLayout ? 24 : 20
            Layout.preferredHeight: !root.isSmallLayout ? 24 : 20
            Layout.alignment: Qt.AlignBottom
            Layout.bottomMargin: !root.isSmallLayout ? 19 : 7

            Image {
                id: icon2

                source: "images/more.svg"
                sourceSize.width: !root.isSmallLayout ? 24 : 20
            }

            MultiEffect {
                anchors.fill: icon2
                source: icon2
                colorization: 1
                colorizationColor: root.isEnabled ? Constants.accentTextColor : "#898989"
            }
        }

    }

    QtObject {
        id: internal

        property int width: 530
        property int height: 276
        property int rightMargin: 60
        property int leftPadding: 16
        property int titleSpacing: 8
        property int spacing: 16
        property int columnMargin: 7
        property int iconSize: 34
        property int switchMargin: 9
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: root
                isSmallLayout: false
            }
            PropertyChanges {
                target: internal
                width: 530
                height: 276
                rightMargin: 53
                leftPadding: 16
                spacing: 16
                titleSpacing: 8
                columnMargin: 7
                iconSize: 34
                switchMargin: 9
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: internal
                width: 306
                height: 177
                rightMargin: 12
                leftPadding: 8
                spacing: 6
                titleSpacing: 4
                columnMargin: 17
                iconSize: 24
                switchMargin: 2
            }
            PropertyChanges {
                target: root
                isSmallLayout: true
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: root
                isSmallLayout: true
            }
            PropertyChanges {
                target: internal
                width: 340
                height: 177
                rightMargin: 34
                leftPadding: 8
                spacing: 3
                titleSpacing: 2
                columnMargin: 7
                iconSize: 24
                switchMargin: 9
            }
        }
    ]
}
