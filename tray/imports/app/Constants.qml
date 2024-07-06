pragma Singleton
import QtQuick

QtObject {
    readonly property int width: 1440
    readonly property int height: 1080

    property string currentView: "RoomsView"

    property bool isBigDesktopLayout: true
    property bool isSmallDesktopLayout: false
    property bool isMobileLayout: false
    property bool isSmallLayout: false

    readonly property font smallTitleFont: Qt.font({
            "family": "Inter",
            "pixelSize": 14,
            "weight": 700
        })

    readonly property font mobileTitleFont: Qt.font({
            "family": "Titillium Web",
            "pixelSize": 24,
            "weight": 600
        })

    readonly property font desktopTitleFont: Qt.font({
            "family": "Titillium Web",
            "pixelSize": 48,
            "weight": 600
        })

    readonly property font desktopSubtitleFont: Qt.font({
            "family": "Titillium Web",
            "pixelSize": 24,
            "weight": 600
        })

    readonly property color backgroundColor: AppSettings.isDarkTheme ? "#000000" : "#EFFCF6"
    readonly property color accentColor: AppSettings.isDarkTheme ? "#002125" : "#FFFFFF"
    readonly property color primaryTextColor: AppSettings.isDarkTheme ? "#FFFFFF" : "#000000"
    readonly property color accentTextColor: AppSettings.isDarkTheme ? "#D9D9D9" : "#898989"
    readonly property color iconColor: AppSettings.isDarkTheme ? "#D9D9D9" : "#00414A"
}
