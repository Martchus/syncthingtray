import QtQml 2.2
import QtQuick 2.8
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQ2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaComponents3.Page {
    header: PlasmaExtras.PlasmoidHeading {
        ToolBar {
            id: toolbar
            width: parent.width
        }
    }

    // define functions to locate the current page and filter
    function findCurrentPage() {
        switch (tabBar.currentIndex) {
        case 0: return directoriesPage
        case 1: return devicesPage
        case 2: return downloadsPage
        case 3: return recentChangesPage
        default: return directoriesPage
        }
    }
    function findCurrentFilter() {
        return findCurrentPage().filter
    }

    // define shortcuts to trigger actions for currently selected item
    function clickCurrentItemButton(buttonName) {
        findCurrentPage().view.clickCurrentItemButton(buttonName)
    }
    Shortcut {
        sequence: "Ctrl+R"
        onActivated: clickCurrentItemButton("rescanButton")
    }
    Shortcut {
        sequence: "Ctrl+P"
        onActivated: clickCurrentItemButton("resumePauseButton")
    }
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: clickCurrentItemButton("openButton")
    }

    FocusScope {
        anchors.fill: parent
        anchors.topMargin: PlasmaCore.Units.smallSpacing * 2

        ColumnLayout {
            anchors.fill: parent

            // ensure keyboard events can be received after initialization
            Component.onCompleted: forceActiveFocus()

            // define custom key handling for switching tabs, selecting items and filtering
            function sendKeyEventToFilter(event) {
                var filter = findCurrentFilter()
                if (!filter || event.text === "" || filter.activeFocus) {
                    return
                }
                if (event.key === Qt.Key_Backspace && filter.text === "") {
                    filter.explicitelyShown = false
                    return
                }
                if (event.matches(StandardKey.Paste)) {
                    filter.paste()
                } else {
                    filter.text = ""
                    filter.text += event.text
                }
                filter.forceActiveFocus()
            }
            Keys.onPressed: {
                // note: event only received after clicking the tab buttons in plasmoidviewer
                // but works as expected in plasmashell
                switch (event.key) {
                case Qt.Key_Up:
                    switch (event.modifiers) {
                    case Qt.NoModifier:
                        // select previous item in current tab
                        findCurrentPage().view.decrementCurrentIndex()
                        break
                    case Qt.ShiftModifier:
                        // select previous connection
                        --plasmoid.nativeInterface.currentConnectionConfigIndex
                        break
                    }
                    break
                case Qt.Key_Down:
                    switch (event.modifiers) {
                    case Qt.NoModifier:
                        // select next item in current tab
                        findCurrentPage().view.incrementCurrentIndex()
                        break
                    case Qt.ShiftModifier:
                        // select previous connection
                        ++plasmoid.nativeInterface.currentConnectionConfigIndex
                        break
                    }
                    break
                case Qt.Key_Left:
                    // select previous tab
                    tabBar.currentIndex = tabBar.currentIndex > 0 ? tabBar.currentIndex - 1 : 3
                    break
                case Qt.Key_Right:
                    // select next tab
                    tabBar.currentIndex = tabBar.currentIndex < 3 ? tabBar.currentIndex + 1 : 0
                    break
                case Qt.Key_Enter:

                    // fallthrough
                case Qt.Key_Return:
                    // toggle expanded state of current item
                    var currentItem = findCurrentPage().view.currentItem
                    if (currentItem) {
                        currentItem.expanded = !currentItem.expanded
                    }
                    break
                case Qt.Key_Escape:
                    var filter = findCurrentFilter()
                    if (filter && filter.text !== "") {
                        // reset filter
                        filter.explicitelyShown = false
                        filter.text = ""
                        event.accepted = true
                    } else {
                        // hide plasmoid
                        plasmoid.expanded = false
                    }
                    break
                case Qt.Key_1:
                    tabBar.currentIndex = 0
                    break
                case Qt.Key_2:
                    tabBar.currentIndex = 1
                    break
                case Qt.Key_3:
                    tabBar.currentIndex = 2
                    break
                case Qt.Key_4:
                    tabBar.currentIndex = 3
                    break
                default:
                    sendKeyEventToFilter(event)
                    return
                }
                event.accepted = true
            }

            // global statistics and traffic
            GridLayout {
                Layout.leftMargin: 5
                Layout.fillWidth: true
                Layout.fillHeight: false
                columns: 5
                rowSpacing: 1
                columnSpacing: 4

                Image {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    height: 16
                    fillMode: Image.PreserveAspectFit
                    source: "image://fa/globe"
                }
                StatisticsView {
                    Layout.leftMargin: 4
                    statistics: plasmoid.nativeInterface.globalStatistics
                    context: qsTr("Global")
                }
                IconLabel {
                    Layout.leftMargin: 10
                    iconSource: "image://fa/cloud-download"
                    iconOpacity: plasmoid.nativeInterface.hasIncomingTraffic ? 1.0 : 0.5
                    text: plasmoid.nativeInterface.incomingTraffic
                    tooltip: qsTr("Global incoming traffic")
                }

                Item {
                    Layout.fillWidth: true
                    Layout.rowSpan: 2
                }
                TinyButton {
                    id: searchButton
                    Layout.fillWidth: false
                    Layout.fillHeight: false
                    Layout.rowSpan: 2
                    icon.source: "image://fa/search"
                    enabled: tabBar.currentIndex === 0
                    opacity: enabled ? 1.0 : 0.25
                    tooltip: qsTr("Toggle filter")
                    onClicked: {
                        var filter = findCurrentFilter()
                        if (!filter) {
                            return
                        }
                        if (!filter.explicitelyShown) {
                            filter.explicitelyShown = true
                            filter.forceActiveFocus()
                        } else {
                            filter.explicitelyShown = false
                            filter.text = ""
                        }
                    }
                }

                Image {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    height: 16
                    fillMode: Image.PreserveAspectFit
                    source: "image://fa/home"
                }
                StatisticsView {
                    Layout.leftMargin: 4
                    statistics: plasmoid.nativeInterface.localStatistics
                    context: qsTr("Local")
                }
                IconLabel {
                    Layout.leftMargin: 10
                    iconSource: "image://fa/cloud-upload"
                    iconOpacity: plasmoid.nativeInterface.hasOutgoingTraffic ? 1.0 : 0.5
                    text: plasmoid.nativeInterface.outgoingTraffic
                    tooltip: qsTr("Global outgoing traffic")
                }
            }

            // tab "widget"
            ColumnLayout {
                id: tabWidget
                spacing: 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                StackLayout {
                    id: tabLayout
                    currentIndex: tabBar.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    DirectoriesPage {
                        id: directoriesPage
                    }
                    DevicesPage {
                        id: devicesPage
                    }
                    DownloadsPage {
                        id: downloadsPage
                    }
                    RecentChangesPage {
                        id: recentChangesPage
                    }
                }
                PlasmaCore.SvgItem {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    elementId: "horizontal-line"
                    svg: PlasmaCore.Svg {
                        imagePath: "widgets/line"
                    }
                }
                PlasmaComponents3.TabBar {
                    id: tabBar
                    position: PlasmaComponents3.TabBar.Footer
                    spacing: units.smallSpacing
                    Layout.alignment: Qt.AlignBottom
                    TabButton {
                        id: dirsTabButton
                        text: qsTr("Directories")
                        icon.source: "image://fa/folder"
                    }
                    TabButton {
                        id: devsTabButton
                        text: qsTr("Devices")
                        icon.source: "image://fa/sitemap"
                    }
                    TabButton {
                        id: downloadsTabButton
                        text: qsTr("Downloads")
                        icon.source: "image://fa/download"
                    }
                    TabButton {
                        id: recentChangesTabButton
                        text: qsTr("History")
                        icon.source: "image://fa/history"
                    }
                }
            }
        }
    }
}
