/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import syncthingtray

ScrollView {
    id: root

    clip: true
    contentWidth: availableWidth

    property alias thermoSettings: thermoSettings
    property alias roomName: thermoSettings.roomNameText
    property alias roomIcon: thermoSettings.roomIconSource
    property alias isActive: thermoSettings.isActive

    property bool isOneColumn
    property int thermostatControlHeight
    property int thermostatControlWidth
    property int delegateHeight
    property int delegateWidth

    background: Rectangle {
        color: Constants.isSmallLayout ? Constants.accentColor : "transparent"
        radius: 12
    }

    GridLayout {
        id: grid

        width: root.width
        height: root.height
        columns: root.isOneColumn ? 1 : 3
        rows: root.isOneColumn ? 10 : 1
        columnSpacing: 24
        rowSpacing: root.isOneColumn ? 12 : 24

        ThermostatSettings {
            id: thermoSettings

            Layout.columnSpan: root.isOneColumn ? 1 : 3
            Layout.rowSpan: root.isOneColumn ? 7 : 1
            Layout.preferredHeight: root.thermostatControlHeight
            Layout.preferredWidth: root.thermostatControlWidth
            Layout.alignment: Qt.AlignHCenter

            model: root.model

            onIsActiveChanged: root.model.active = isActive
        }

        ThermostatInfo {
            Layout.preferredHeight: root.delegateHeight
            Layout.preferredWidth: root.delegateWidth
            Layout.alignment: Qt.AlignHCenter
        }

        HumidityInfo {
            Layout.preferredHeight: root.delegateHeight
            Layout.preferredWidth: root.delegateWidth
            Layout.alignment: Qt.AlignHCenter

            humidityValuesModel: root.model.humidityStats
        }

        EnergyInfo {
            Layout.preferredHeight: root.delegateHeight
            Layout.preferredWidth: root.delegateWidth
            Layout.alignment: Qt.AlignHCenter

            energyValuesModel: root.model.energyStats
        }
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: root
                thermostatControlHeight: 673
                thermostatControlWidth: 1094
                delegateHeight: 182
                delegateWidth: 350
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: root
                delegateWidth: 327
                delegateHeight: 100
                thermostatControlHeight: 694
                thermostatControlWidth: 327
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: root
                delegateWidth: 332
                delegateHeight: 80
                thermostatControlHeight: 250
                thermostatControlWidth: 400
            }
        }
    ]
}
