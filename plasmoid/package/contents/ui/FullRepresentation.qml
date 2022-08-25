import QtQml 2.2
import QtQuick 2.8
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.15 as QQ2
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 3.0 as PlasmaComponents3

PlasmaExtras.Representation {
    // disable margins as they don't look good together with the scroll view
    // note: Would be collapsed automatically if the scroll view was the immediate content item.
    collapseMarginsHint: true

    // header ("toolbar" with buttons and combo box) and footer ("tabbar")
    header: PlasmaExtras.PlasmoidHeading {
        ToolBar {
            id: toolbar
            width: parent.width
        }
    }
    footer: PlasmaExtras.PlasmoidHeading {
        spacing: 0
        topPadding: 0
        height: units.iconSizes.medium
        PlasmaComponents3.TabBar {
            id: tabBar
            readonly property double buttonWidth: parent.width / count
            position: PlasmaComponents3.TabBar.Footer
            Layout.fillWidth: true
            Layout.fillHeight: true
            TabButton {
                id: dirsTabButton
                text: qsTr("Directories")
                icon.source: plasmoid.nativeInterface.faUrl + "folder"
                width: tabBar.buttonWidth
            }
            TabButton {
                id: devsTabButton
                text: qsTr("Devices")
                icon.source: plasmoid.nativeInterface.faUrl + "sitemap"
                width: tabBar.buttonWidth
            }
            TabButton {
                id: downloadsTabButton
                text: qsTr("Downloads")
                icon.source: plasmoid.nativeInterface.faUrl + "download"
                width: tabBar.buttonWidth
            }
            TabButton {
                id: recentChangesTabButton
                text: qsTr("History")
                icon.source: plasmoid.nativeInterface.faUrl + "history"
                width: tabBar.buttonWidth
            }
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

    // main contents
    FocusScope {
        anchors.fill: parent
        anchors.topMargin: PlasmaCore.Units.smallSpacing * 2

        TinyButton {
            id: searchButton
            anchors.right: mainLayout.right
            anchors.verticalCenter: infoLayout.verticalCenter
            icon.source: plasmoid.nativeInterface.faUrl + "search"
            width: PlasmaCore.Units.iconSizes.smallMedium
            height: width
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

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent
            spacing: 0

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
                id: infoLayout
                Layout.leftMargin: 5
                Layout.bottomMargin: 2
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.maximumWidth: parent.width
                columns: 4
                rowSpacing: 1
                columnSpacing: 4

                Image {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    height: 16
                    fillMode: Image.PreserveAspectFit
                    source: plasmoid.nativeInterface.faUrl + "globe"
                    cache: false
                }
                StatisticsView {
                    Layout.leftMargin: 4
                    statistics: plasmoid.nativeInterface.globalStatistics
                    context: qsTr("Global")
                }
                IconLabel {
                    Layout.leftMargin: 5
                    iconSource: plasmoid.nativeInterface.faUrl + "cloud-download"
                    iconOpacity: plasmoid.nativeInterface.hasIncomingTraffic ? 1.0 : 0.5
                    text: plasmoid.nativeInterface.incomingTraffic
                    tooltip: qsTr("Global incoming traffic")
                }

                Item {
                    Layout.fillWidth: true
                    Layout.rowSpan: 2
                }
                Image {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    height: 16
                    fillMode: Image.PreserveAspectFit
                    source: plasmoid.nativeInterface.faUrl + "home"
                    cache: false
                }
                StatisticsView {
                    Layout.leftMargin: 4
                    statistics: plasmoid.nativeInterface.localStatistics
                    context: qsTr("Local")
                }
                IconLabel {
                    Layout.leftMargin: 5
                    iconSource: plasmoid.nativeInterface.faUrl + "cloud-upload"
                    iconOpacity: plasmoid.nativeInterface.hasOutgoingTraffic ? 1.0 : 0.5
                    text: plasmoid.nativeInterface.outgoingTraffic
                    tooltip: qsTr("Global outgoing traffic")
                }
            }

            // tab content
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
        }
    }
}
