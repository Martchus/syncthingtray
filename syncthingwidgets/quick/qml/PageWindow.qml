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
    onClosing: (event) => {
        if (!pageWindow.forceClose) {
            for (let i = 0, depth = stackView.depth; i !== depth; ++i) {
                if (stackView.get(i).hasUnsavedChanges) {
                    discardChangesDialog.open();
                    event.accepted = false;
                    return;
                }
            }
        }
        pageWindow.destroy();
    }
    Material.theme: theming.Material.theme
    Material.primary: theming.Material.primary
    Material.accent: theming.Material.accent
    Component.onCompleted: {
        page.stackView = stackView;
        page.background = QuickUI.makePageBackground();
        page.forceActiveFocus();
    }
    footer: Pane {
        padding: 0
        DialogButtonBox {
            width: parent.width
            IconOnlyButton {  // using IconOnlyButton because normal Button does not show icon with Breeze style
                visible: stackView.depth > 1
                icon.source: QuickUI.faUrlBase + "arrow-left"
                icon.width: QuickUI.iconSize
                icon.height: QuickUI.iconSize
                text: qsTr("Back")
                onClicked: stackView.pop()
            }
            IconOnlyButton {
                id: extraActionsMenuButton
                visible: currentPage.extraActions?.length > 0
                icon.source: QuickUI.faUrlBase + "ellipsis-v"
                icon.width: QuickUI.iconSize
                icon.height: QuickUI.iconSize
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
                icon.source: Utils.winUI ? "" : QuickUI.faUrlBase + "times"
                text: qsTr("Close")
                flat: Utils.flatDialogButtons
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
                            flat: Utils.flatDialogButtons
                            icon.source: Utils.winUI ? "" : modelData.icon.source
                            onClicked: modelData.trigger()
                        }
                    }
                }
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: page
        Keys.onEscapePressed: (event) => pageWindow.close()
    }
    DiscardChangesDialog {
        id: discardChangesDialog
        meta: pageWindow.meta
        pageStack: pageStack
        prompt: qsTr("Do you really want to close without applying changes?")
        onAccepted: {
            pageWindow.forceClose = true;
            pageWindow.close();
        }
    }

    required property Page page
    property bool forceClose: false
    property alias currentPage: stackView.currentItem
    readonly property Theming theming: Theming {
        currentPage: stackView.currentItem
    }
    readonly property Meta meta: Meta {
    }
}
