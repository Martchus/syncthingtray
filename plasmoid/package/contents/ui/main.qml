import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: syncthingApplet

    Plasmoid.switchWidth: units.gridUnit * 20
    Plasmoid.switchHeight: units.gridUnit * 30
    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    Plasmoid.compactRepresentation: CompactRepresentation { }
    Plasmoid.fullRepresentation: FullRepresentation {
        focus: true
    }
    Plasmoid.icon: "syncthingtray"
    Plasmoid.toolTipMainText: plasmoid.nativeInterface.statusText
    Plasmoid.toolTipSubText: plasmoid.nativeInterface.additionalStatusText
    Plasmoid.hideOnWindowDeactivate: true

    function action_showWebUI() {
        plasmoid.nativeInterface.showWebUI()
    }

    function action_showSettings() {
        plasmoid.nativeInterface.showConnectionSettingsDlg()
    }

    function action_rescanAllDirs() {
        plasmoid.nativeInterface.connection.rescanAllDirs()
    }

    function action_showLog() {
        plasmoid.nativeInterface.showLog()
    }

    function action_showAboutDialog() {
        plasmoid.nativeInterface.showAboutDialog()
    }

    Component.onCompleted: {
        plasmoid.removeAction("configure");
        plasmoid.setAction("showWebUI", qsTr("Web UI"), "internet-web-browser");
        plasmoid.setAction("showSettings", qsTr("Settings"), "configure");
        plasmoid.setAction("showLog", qsTr("Log"), "text-x-generic");
        plasmoid.setAction("showAboutDialog", qsTr("About"), "help-about");
    }
}
