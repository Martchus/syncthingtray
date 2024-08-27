import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Main

Page {
    title: qsTr("Recent changes")
    Layout.fillWidth: true
    Layout.fillHeight: true
    ListView {
        id: mainView
        anchors.fill: parent
        model: DelegateModel {
            model: app.changesModel
            delegate: ItemDelegate {
                width: mainView.width
                onClicked: app.openPath(modelData.directoryId, modelData.path)
                onPressAndHold: app.copyPath(modelData.directoryId, modelData.path)
                contentItem: RowLayout {
                    spacing: 10
                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: app.faUrlBase + ((modelData.action === "deleted") ? ("trash-o") : (modelData.itemType === "file" ? "file-o" : "folder-o"))
                        width: 16
                        height: 16
                    }
                    Label {
                        Layout.fillWidth: true
                        text: [modelData.directoryName || modelData.directoryId, modelData.path].join(": ")
                        elide: Text.ElideRight
                    }
                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: app.faUrlBase + "calendar"
                        width: 16
                        height: 16
                    }
                    Label {
                        text: modelData.eventTime
                        elide: Text.ElideRight
                    }
                    Icon {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: modelData.actionIcon
                        width: 16
                        height: 16
                    }
                    Label {
                        text: modelData.modifiedBy
                        elide: Text.ElideRight
                    }
                }
                required property var modelData
            }
        }
        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
