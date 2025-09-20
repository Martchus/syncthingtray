import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ItemDelegate {
    width: listView.width
    onPressAndHold: App.copyText(`${modelData.when}\n${modelData.url}: ${modelData.message}`)
    contentItem: GridLayout {
        columns: 2
        columnSpacing: 10
        ForkAwesomeIcon {
            iconName: "calendar"
        }
        Label {
            Layout.fillWidth: true
            text: modelData.when
            elide: Text.ElideRight
            font.weight: Font.Light
        }
        ForkAwesomeIcon {
            iconName: "exclamation-triangle"
        }
        Label {
            Layout.fillWidth: true
            text: modelData.message
            wrapMode: Text.WordWrap
            font.weight: Font.Light
        }
    }
    required property var modelData
}
