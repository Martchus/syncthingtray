/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import appCustomControls
import syncthingtray

ScrollView {
    id: scrollView

    required property var model

    property bool isOneColumn: false
    property bool isBackgroundVisible: false
    property int statisticsChartHeight: 647
    property int statisticsChartWidth: 1098
    property int delegateHeight: 182
    property int delegateWidth: 350

    clip: true
    padding: 0
    contentWidth: availableWidth

    background: Rectangle {
        color: Constants.accentColor
        visible: scrollView.isBackgroundVisible
        radius: 12
    }

    GridLayout {
        id: grid

        width: scrollView.width
        height: scrollView.height

        columns: scrollView.isOneColumn ? 1 : 3
        rows: scrollView.isOneColumn ? 8 : 1

        columnSpacing: 24
        rowSpacing: scrollView.isOneColumn ? 12 : 24

        Pane {
            id: statistics

            leftPadding: 53
            rightPadding: 53
            topPadding: 23
            bottomPadding: 43

            Layout.columnSpan: scrollView.isOneColumn ? 1 : 3
            Layout.rowSpan: scrollView.isOneColumn ? 5 : 1

            Layout.preferredHeight: scrollView.statisticsChartHeight
            Layout.preferredWidth: scrollView.statisticsChartWidth
            Layout.alignment: Qt.AlignHCenter

            background: Rectangle {
                radius: 12
                color: Constants.accentColor
            }

            StatisticsChart {
                id: statisticsChart

                anchors.fill: parent
                energyValuesModel: model.energyStats
                tempValuesModel: model.tempStats
            }
        }

        TemperatureInfo {
            id: tempInfo

            Layout.preferredHeight: scrollView.delegateHeight
            Layout.preferredWidth: scrollView.delegateWidth
            Layout.alignment: Qt.AlignHCenter
            maxTempValue: statisticsChart.maxValue
            minTempValue: statisticsChart.minValue
            avgTempValue: statisticsChart.avgValue
        }

        HumidityInfo {
            id: humidityInfo
            Layout.preferredHeight: scrollView.delegateHeight
            Layout.preferredWidth: scrollView.delegateWidth
            Layout.alignment: Qt.AlignHCenter

            humidityValuesModel: model.humidityStats
        }

        EnergyInfo {
            id: energyInfo
            Layout.preferredHeight: scrollView.delegateHeight
            Layout.preferredWidth: scrollView.delegateWidth
            Layout.alignment: Qt.AlignHCenter

            energyValuesModel: model.energyStats
        }
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isBigDesktopLayout || Constants.isSmallDesktopLayout
            PropertyChanges {
                target: statistics
                leftPadding: 53
                rightPadding: 53
                topPadding: 23
                bottomPadding: 43
            }
            PropertyChanges {
                target: scrollView
                isBackgroundVisible: false
                delegateWidth: 350
                delegateHeight: 182
                statisticsChartWidth: 1098
                statisticsChartHeight: 647
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: statistics
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 43
            }
            PropertyChanges {
                target: scrollView
                isBackgroundVisible: false
                delegateWidth: 327
                delegateHeight: 100
                statisticsChartWidth: 327
                statisticsChartHeight: 383
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: statistics
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 43
            }
            PropertyChanges {
                target: scrollView
                isBackgroundVisible: true
                delegateWidth: 332
                delegateHeight: 80
                statisticsChartWidth: 401
                statisticsChartHeight: 280
            }
        }
    ]
}
