import QtQuick
import QtQuick.Controls
import QtCharts
import syncthingtray

Pane {
    width: 900
    height: 580

    padding: 0

    property var energyValuesModel
    property var tempValuesModel

    property var energyValues: {
        var enrg = [];
        for (let i = 0; i < energyValuesModel.count; ++i) {
            enrg.push(energyValuesModel.get(i).enrg);
        }
        return enrg;
    }

    property var tempValues: {
        var temp = [];
        for (let i = 0; i < tempValuesModel.count; ++i) {
            temp.push(tempValuesModel.get(i).tmp);
        }
        return temp;
    }

    property real maxValue
    property real minValue
    property real avgValue

    background: Rectangle {
        radius: 12
        color: Constants.accentColor
    }

    ValuesAxis {
        id: barChartAxisY

        min: 0
        max: 2000
        labelsColor: internal.energyBarColor
        labelsFont.family: "Titillium Web"
        labelsFont.pixelSize: internal.axisFontSize
    }

    ValuesAxis {
        id: splineChartAxisY

        min: 0
        max: 40
        tickAnchor: 5
        tickInterval: 10
        tickType: ValuesAxis.TicksDynamic
        labelsColor: internal.splineChartColor
        labelsFont.family: "Titillium Web"
        lineVisible: false
        labelsFont.pixelSize: internal.axisFontSize
    }

    ValuesAxis {
        id: splineChartAxisX

        visible: false
        min: 0
        max: 365
    }

    BarCategoryAxis {
        id: barChartAxisX

        labelsColor: internal.energyBarColor
        gridVisible: false
        labelsFont.family: "Titillium Web"
        labelsFont.pixelSize: internal.axisFontSize
        truncateLabels: false
        categories: [qsTr("Jan"), qsTr("Feb"), qsTr("Mar"), qsTr("Apr"), qsTr("May"), qsTr("Jun"), qsTr("Jul"), qsTr("Aug"), qsTr("Sep"), qsTr("Oct"), qsTr("Nov"), qsTr("Dec")]
    }

    ChartView {
        id: chart

        anchors.fill: parent
        antialiasing: true

        margins.left: 0
        margins.right: 0
        margins.top: 0
        margins.bottom: 0

        legend.alignment: Qt.AlignTop
        legend.markerShape: Legend.MarkerShapeCircle
        legend.font.family: "Titillium Web"
        legend.font.weight: 400
        legend.font.pixelSize: internal.fontSize
        legend.labelColor: Constants.primaryTextColor

        dropShadowEnabled: internal.dropShadowEnabled
        backgroundColor: Constants.accentColor

        BarSeries {
            id: mySeries

            axisX: barChartAxisX
            axisY: barChartAxisY
            barWidth: internal.barWidth

            BarSet {
                id: energySet

                label: "Energy Usage [wh]"
                color: internal.energyBarColor
                borderWidth: 0
                values: energyValues
            }
        }

        SplineSeries {
            id: spline

            name: "Temperature [°C]"
            color: internal.splineChartColor
            width: internal.lineWidth

            axisX: splineChartAxisX
            axisYRight: splineChartAxisY

            Component.onCompleted: function () {
                for (var i = 0; i < tempValues.length; i++) {
                    spline.append(i * 7, tempValues[i]);
                }
            }
        }
    }

    QtObject {
        id: internal

        property int fontSize: 14
        property int axisFontSize: 14
        property int lineWidth: 5
        property real barWidth: 0.5
        property bool dropShadowEnabled: true
        readonly property color energyBarColor: AppSettings.isDarkTheme ? "#2CDE85" : "#00414A"
        readonly property color splineChartColor: AppSettings.isDarkTheme ? "#D9D9D9" : "#2CDE85"
    }

    states: [
        State {
            name: "desktopLayout"
            when: Constants.isSmallDesktopLayout || Constants.isBigDesktopLayout
            PropertyChanges {
                target: internal
                fontSize: 14
                axisFontSize: 14
                lineWidth: 5
                barWidth: 0.5
                dropShadowEnabled: true
            }
        },
        State {
            name: "mobileLayout"
            when: Constants.isMobileLayout
            PropertyChanges {
                target: internal
                fontSize: 10
                axisFontSize: 8
                lineWidth: 2
                barWidth: 0.7
                dropShadowEnabled: false
            }
        },
        State {
            name: "smallLayout"
            when: Constants.isSmallLayout
            PropertyChanges {
                target: internal
                fontSize: 8
                axisFontSize: 10
                lineWidth: 2
                barWidth: 0.6
                dropShadowEnabled: false
            }
        }
    ]

    Component.onCompleted: function () {
        maxValue = Math.max(...tempValues).toFixed(1);
        minValue = Math.min(...tempValues).toFixed(1);
        avgValue = (tempValues.reduce((a, b) => a + b, 0) / tempValues.length).toFixed(1);
    }
}
