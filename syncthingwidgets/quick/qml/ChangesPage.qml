import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    title: qsTr("Recent changes")
    Layout.fillWidth: true
    Layout.fillHeight: true
    CustomListView {
        id: mainView
        anchors.fill: parent
        model: DelegateModel {
            model: App.changesModel
            delegate: ItemDelegate {
                width: mainView.width
                onClicked: App.openPath(modelData.directoryId, modelData.path)
                onPressAndHold: App.copyPath(modelData.directoryId, modelData.path)
                contentItem: GridLayout {
                    id: gridLayout
                    columns: width < 500 ? 2 : 8
                    columnSpacing: 10
                    ForkAwesomeIcon {
                        iconName: (modelData.action === "deleted") ? ("trash-o") : (modelData.itemType === "file" ? "file-o" : "folder-o")
                    }
                    Label {
                        Layout.fillWidth: true
                        text: [modelData.directoryName || modelData.directoryId, modelData.path].join(": ")
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                    ForkAwesomeIcon {
                        iconName: "calendar"
                    }
                    Label {
                        Layout.preferredWidth: Math.max(implicitWidth, parent.width / 5)
                        text: modelData.eventTime
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                    Icon {
                        Layout.preferredWidth: App.iconSize
                        Layout.preferredHeight: App.iconSize
                        source: modelData.actionIcon
                        width: 16
                        height: 16
                    }
                    Label {
                        Layout.preferredWidth: Math.max(implicitWidth, parent.width / 5)
                        text: modelData.modifiedBy
                        elide: Text.ElideRight
                        font.weight: Font.Light
                    }
                }
                required property var modelData
            }
        }
    }
}
