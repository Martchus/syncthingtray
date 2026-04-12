import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

ApplicationWindow {
    id: pageWindow
    visible: true
    width: 700
    height: 500
    title: (page.currentItem?.title ?? page.title) + " - " + meta.title + " - WORK IN PROGRESS"
    font: theming.font
    flags: QuickUI.extendedClientArea ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : (Qt.Window)
    leftPadding: 0
    rightPadding: 0
    footer: DialogButtonBox {
        CustomToolButton {
            visible: stackView.depth > 1
            icon.source: QuickUI.faUrlBase + "arrow-left"
            text: qsTr("Back")
            onClicked: stackView.pop()
        }
        CustomToolButton {
            id: extraActionsMenuButton
            visible: currentPage.extraActions?.length > 0
            icon.source: QuickUI.faUrlBase + "ellipsis-v"
            text: qsTr("More")
            onClicked: currentPage?.showExtraActions() ?? extraActionsMenu.showCenteredIn(extraActionsMenuButton)
            CustomMenu {
                id: extraActionsMenu
                MenuItemInstantiator {
                    menu: extraActionsMenu
                    model: {
                        const extraActions = currentPage.showExtraActions === undefined ? currentPage.extraActions : undefined;
                        return extraActions ?? [];
                    }
                }
            }
        }
        Button {
            display: AbstractButton.TextBesideIcon
            icon.width: QuickUI.iconSize
            icon.height: QuickUI.iconSize
            icon.source: QuickUI.faUrlBase + "times"
            text: qsTr("Abort")
            onClicked: pageWindow.close()
        }
        Repeater {
            model: currentPage.actions
            DelegateChooser {
                role: "enabled"
                DelegateChoice {
                    roleValue: true
                    Button {
                        required property Action modelData
                        display: AbstractButton.TextBesideIcon
                        icon.width: QuickUI.iconSize
                        icon.height: QuickUI.iconSize
                        enabled: modelData.enabled
                        text: modelData.text
                        icon.source: modelData.icon.source
                        onClicked: modelData.trigger()
                    }
                }
            }
        }
    }

    Component.onCompleted: page.stackView = stackView

    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: page
    }

    required property Page page
    property alias currentPage: stackView.currentItem
    readonly property Theming theming: Theming {
        pageStack: null
    }
    readonly property Meta meta: Meta {
    }
}
