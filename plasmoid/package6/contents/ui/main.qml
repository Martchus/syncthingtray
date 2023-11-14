import QtQuick 2.2
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.20 as Kirigami

PlasmoidItem {
    id: syncthingApplet

    Plasmoid.title: "Syncthing"
    Plasmoid.icon: "syncthing"
    switchWidth: Kirigami.Units.gridUnit * Plasmoid.size.width
    switchHeight: Kirigami.Units.gridUnit * Plasmoid.size.height
    compactRepresentation: CompactRepresentation {}
    fullRepresentation: FullRepresentation {
        Layout.minimumWidth: syncthingApplet.switchWidth
        Layout.minimumHeight: syncthingApplet.switchHeight
    }

    toolTipMainText: Plasmoid.statusText
    toolTipSubText: Plasmoid.additionalStatusText
    toolTipItem: ToolTipView {}

    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: qsTr("Open Syncthing")
            icon.name: "syncthing"
            onTriggered: Plasmoid.showWebUI()
        },
        PlasmaCore.Action {
            text: qsTr("Rescan all folders")
            icon.name: "folder-sync"
            onTriggered: Plasmoid.connection.rescanAllDirs()
        },
        PlasmaCore.Action {
            text: qsTr("Show own device ID")
            icon.name: "view-barcode-qr"
            onTriggered: Plasmoid.showOwnDeviceId()
        },
        PlasmaCore.Action {
            text: qsTr("Restart Syncthing")
            icon.name: "system-reboot"
            onTriggered: Plasmoid.connection.restart()
        },
        PlasmaCore.Action {
            text: qsTr("Log")
            icon.name: "text-x-generic"
            onTriggered: Plasmoid.showLog()
        },
        PlasmaCore.Action {
            text: qsTr("Internal errors")
            icon.name: "data-error"
            onTriggered: Plasmoid.showInternalErrorsDialog()
            visible: Plasmoid.hasInternalErrors
        },
        PlasmaCore.Action {
            text: qsTr("About")
            icon.name: "help-about"
            onTriggered: Plasmoid.showAboutDialog()
        }
    ]

    PlasmaCore.Action {
        id: configureAction
        text: qsTr("Settings")
        icon.name: "configure"
        onTriggered: Plasmoid.showSettingsDlg()
    }

    Component.onCompleted: {
        Plasmoid.initEngine(this)
        Plasmoid.setInternalAction("configure", configureAction)
    }
}
