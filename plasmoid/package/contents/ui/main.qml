import QtQuick 2.2
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.kquickcontrolsaddons 2.0

Item {
    id: syncthingApplet

    Plasmoid.switchWidth: units.gridUnit * (plasmoid.nativeInterface.size.width + 1)
    Plasmoid.switchHeight: units.gridUnit * (plasmoid.nativeInterface.size.height + 1)
    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    Plasmoid.compactRepresentation: CompactRepresentation {}
    Plasmoid.fullRepresentation: FullRepresentation {
        Layout.minimumWidth: units.gridUnit * plasmoid.nativeInterface.size.width
        Layout.minimumHeight: units.gridUnit * plasmoid.nativeInterface.size.height
    }
    Plasmoid.icon: "syncthingtray"
    Plasmoid.toolTipMainText: plasmoid.nativeInterface.statusText
    Plasmoid.toolTipSubText: plasmoid.nativeInterface.additionalStatusText
    Plasmoid.toolTipItem: ToolTipView {}

    Plasmoid.hideOnWindowDeactivate: true

    function action_showWebUI() {
        plasmoid.nativeInterface.showWebUI()
    }

    function action_configure() {
        plasmoid.nativeInterface.showSettingsDlg()
    }

    function action_showOwnId() {
        plasmoid.nativeInterface.showOwnDeviceId()
    }

    function action_rescanAllDirs() {
        plasmoid.nativeInterface.connection.rescanAllDirs()
    }

    function action_restartSyncthing() {
        plasmoid.nativeInterface.connection.restart()
    }

    function action_showLog() {
        plasmoid.nativeInterface.showLog()
    }

    function action_showErrors() {
        plasmoid.nativeInterface.showInternalErrorsDialog()
    }

    function action_showAboutDialog() {
        plasmoid.nativeInterface.showAboutDialog()
    }

    Component.onCompleted: {
        plasmoid.nativeInterface.initEngine(this)
        plasmoid.removeAction("configure")
        plasmoid.setAction("showWebUI", qsTr("Open Syncthing"), "syncthing")
        plasmoid.setAction("configure", qsTr("Settings"), "configure")
        plasmoid.setAction("rescanAllDirs", qsTr("Rescan all directories"),
                           "folder-sync")
        plasmoid.setAction("showOwnId", qsTr("Show own device ID"),
                           "view-barcode-qr")
        plasmoid.setAction("restartSyncthing", qsTr("Restart Syncthing"),
                           "system-reboot")
        plasmoid.setAction("showLog", qsTr("Log"), "text-x-generic")
        plasmoid.setAction("showErrors", qsTr("Internal errors"), "data-error")
        plasmoid.action("showErrors").visible = Qt.binding(() => { return plasmoid.nativeInterface.hasInternalErrors })
        plasmoid.setAction("showAboutDialog", qsTr("About"), "help-about")
    }
}
