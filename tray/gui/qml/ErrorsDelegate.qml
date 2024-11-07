import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Main

ItemDelegate {
    width: listView.width
    onPressAndHold: App.copyText(`${modelData.when}\n${modelData.url}: ${modelData.message}`)
    contentItem: GridLayout {
        columns: 2
        columnSpacing: 10
        Image {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: App.faUrlBase + "calendar"
            width: 16
            height: 16
        }
        Label {
            Layout.fillWidth: true
            text: modelData.when
            elide: Text.ElideRight
            font.weight: Font.Light
        }
        Image {
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: App.faUrlBase + "exclamation-triangle"
            width: 16
            height: 16
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
