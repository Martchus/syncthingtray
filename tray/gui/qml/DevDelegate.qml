import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

ExpandableDelegate {
    id: mainDelegateModel
    delegate: ExpandableItemDelegate {
        mainView: mainDelegateModel.mainView
        actions: [
            Action {
                text: modelData.paused ? qsTr("Resume") : qsTr("Pause")
                enabled: !modelData.isThisDevice
                icon.source: app.faUrlBase + (modelData.paused ? "play" : "pause")
                onTriggered: (source) => app.connection[modelData.paused ? "resumeDevice" : "pauseDevice"]([modelData.devId])
            }
        ]
        extraActions: [
            Action {
                text: qsTr("Advanced config")
                icon.source: app.faUrlBase + "cogs"
                onTriggered: (source) => mainView.stackView.push("AdvancedDevConfigPage.qml", {devName: modelData.name, devId: modelData.devId, stackView: mainView.stackView}, StackView.PushTransition)
            }
        ]
    }
}
