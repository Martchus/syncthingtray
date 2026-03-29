import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ApplicationWindow {
    id: appWindow
    visible: true
    width: 700
    height: 500
    title: meta.title
    font: theming.font
    flags: QuickUI.extendedClientArea ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : (Qt.Window)
    leftPadding: 0
    rightPadding: 0
    header: Label {
        text: "The integration of the Quick GUI into the desktop UI is still work in progress."
    }
    footer: Label {
        text: SyncthingData.website
    }

    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.ForwardButton | Qt.BackButton
        propagateComposedEvents: true
        onClicked: (event) => {
            const button = event.button;
            if (button === Qt.BackButton) {
            } else if (button === Qt.ForwardButton) {
            }
        }
    }

    readonly property Theming theming: Theming {
        pageStack: null
    }
    readonly property Meta meta: Meta {
    }
}
