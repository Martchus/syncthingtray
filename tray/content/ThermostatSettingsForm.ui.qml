/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import controls
import app

Pane {
    id: root

    required property var model
    property alias buttonGroup: buttonGroup
    property alias powerButton: powerButton
    property alias powerToggle: powerToggle
    property alias roomIconSource: roomIcon.source
    property alias roomNameText: roomName.text
    property bool isActive: false

    padding: 0

    background: Rectangle {
        color: Constants.accentColor
        radius: 12
    }

    Row {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: internal.leftMargin
        anchors.topMargin: internal.topMargin
        spacing: internal.headerSpacing

        Item {
            width: internal.iconSize
            height: internal.iconSize

            Image {
                id: roomIcon
                source: "images/" + root.model.iconName

                sourceSize.width: internal.iconSize
                sourceSize.height: internal.iconSize
            }

            MultiEffect {
                anchors.fill: roomIcon
                source: roomIcon
                colorization: 1
                colorizationColor: Constants.iconColor
            }
        }

        Label {
            id: roomName
            text: qsTr("Living room")
            font.pixelSize: internal.headerSize
            font.weight: 600
            font.family: "Titillium Web"
            color: Constants.primaryTextColor
        }
    }

    CustomSwitch {
        id: powerToggle
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 12
        anchors.rightMargin: 12
        checked: root.isActive
        Connections {
            function onCheckedChanged() {
                root.isActive = powerToggle.checked
            }
        }
    }

    ThermostatControl {
        id: thermostat
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: internal.thermostatTopMargin
        enabled: root.isActive

        currentTemp: root.model.temp
        targetTemp: root.model.thermostatTemp
        onTargetTempChanged: root.model.thermostatTemp = targetTemp
    }

    ButtonGroup {
        id: buttonGroup
    }

    Column {
        id: leftButtons

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 100
        spacing: 25

        Repeater {
            model: [qsTr("Cool"), qsTr("Fan"), qsTr("Auto")]

            CustomRoundButton {
                width: internal.buttonWidth
                height: internal.buttonHeight
                text: modelData
                font.pixelSize: 18
                icon.source: "images/" + modelData
                icon.width: internal.optionIconSize
                icon.height: internal.optionIconSize
                LayoutMirroring.enabled: true
                radius: internal.radius
                ButtonGroup.group: buttonGroup
                enabled: root.isActive
                checked: enabled && root.model.mode == modelData
            }
        }
    }

    Column {
        id: rightButtons
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 100
        spacing: 25

        Repeater {
            model: [qsTr("Heat"), qsTr("Dry"), qsTr("Eco")]

            CustomRoundButton {
                width: internal.buttonWidth
                height: internal.buttonHeight
                text: modelData
                font.pixelSize: 18
                icon.source: "images/" + modelData
                icon.width: internal.optionIconSize
                icon.height: internal.optionIconSize
                radius: internal.radius
                ButtonGroup.group: buttonGroup
                enabled: root.isActive
                checked: enabled && root.model.mode == modelData
            }
        }
    }

    CustomRoundButton {
        id: powerButton
        height: 80
        width: 80
        radius: 65
        text: qsTr("Power")
        font.pixelSize: 18
        icon.source: "images/power.svg"
        icon.width: 40
        icon.height: 40
        display: AbstractButton.IconOnly
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 50
        checked: root.isActive
        Connections {
            function onCheckedChanged() {
                root.isActive = powerButton.checked
            }
        }
    }

    QtObject {
        id: internal

        property int buttonWidth: 130
        property int buttonHeight: 66
        property int radius: 20
        property int leftMargin: 20
        property int topMargin: 16
        property int iconSize: 34
        property int optionIconSize: 42
        property int headerSpacing: 24
        property int headerSize: 24
        property int thermostatTopMargin: 14
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: internal
                buttonHeight: 66
                buttonWidth: 130
                radius: 20
                leftMargin: 20
                topMargin: 16
                iconSize: 34
                optionIconSize: 42
                headerSpacing: 24
                headerSize: 24
                thermostatTopMargin: 14
            }
            PropertyChanges {
                target: powerButton
                width: 80
                height: 80
                radius: 40
                visible: true
                anchors.bottomMargin: 50
                icon.width: 40
                icon.height: 40
            }
            PropertyChanges {
                target: powerToggle
                visible: false
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: internal
                buttonHeight: 50
                buttonWidth: 110
                radius: 12
                leftMargin: 16
                topMargin: 16
                iconSize: 24
                optionIconSize: 30
                headerSpacing: 16
                headerSize: 18
                thermostatTopMargin: 60
            }
            PropertyChanges {
                target: leftButtons
                anchors.leftMargin: 46 - root.leftPadding
                spacing: 14
            }
            AnchorChanges {
                target: leftButtons
                anchors.verticalCenter: undefined
                anchors.top: thermostat.bottom
            }
            PropertyChanges {
                target: rightButtons
                anchors.rightMargin: 46 - root.rightPadding
                spacing: 14
            }
            AnchorChanges {
                target: rightButtons
                anchors.verticalCenter: undefined
                anchors.top: thermostat.bottom
            }
            PropertyChanges {
                target: powerButton
                width: 60
                height: 60
                radius: 30
                visible: true
                anchors.bottomMargin: 32
                icon.width: 30
                icon.height: 30
            }
            PropertyChanges {
                target: powerToggle
                visible: false
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: internal
                buttonHeight: 42
                buttonWidth: 42
                radius: 21
                leftMargin: 14
                topMargin: 11
                iconSize: 24
                optionIconSize: 24
                headerSpacing: 12
                headerSize: 18
                thermostatTopMargin: 19
            }
            PropertyChanges {
                target: powerButton
                visible: false
            }
            PropertyChanges {
                target: leftButtons
                anchors.leftMargin: 14
                spacing: 9
            }
            AnchorChanges {
                target: leftButtons
                anchors.verticalCenter: thermostat.verticalCenter
                anchors.top: undefined
            }
            PropertyChanges {
                target: rightButtons
                anchors.rightMargin: 14
                spacing: 9
            }
            AnchorChanges {
                target: rightButtons
                anchors.verticalCenter: thermostat.verticalCenter
                anchors.top: undefined
            }
            PropertyChanges {
                target: powerToggle
                visible: true
            }
        }
    ]
}
