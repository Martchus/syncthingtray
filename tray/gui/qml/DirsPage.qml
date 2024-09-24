import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        title: qsTr("Folders")
        Layout.fillWidth: true
        Layout.fillHeight: true
        DirListView {
            mainModel: app.dirModel
            stackView: stackView
        }
        property list<Action> actions: [
            Action {
                text: qsTr("Add folder")
                icon.source: app.faUrlBase + "plus"
            }
        ]
        property list<Action> extraActions: [
            Action {
                text: qsTr("Pause all folders")
                icon.source: app.faUrlBase + "pause"
                onTriggered: (source) => app.connection.pauseAllDirs()
            },
            Action {
                text: qsTr("Resume all folders")
                icon.source: app.faUrlBase + "play"
                onTriggered: (source) => app.connection.resumeAllDirs()
            },
            Action {
                text: qsTr("Rescan all folders")
                icon.source: app.faUrlBase + "refresh"
                onTriggered: (source) => app.connection.rescanAllDirs()
            }
        ]
    }
}
