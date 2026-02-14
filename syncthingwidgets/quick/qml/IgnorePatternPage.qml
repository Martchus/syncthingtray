import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Main

Page {
    id: page
    title: qsTr("Ignore patterns of \"%1\"").arg(dirName)
    Component.onCompleted: App.loadIgnorePatterns(dirId, textArea)
    property list<Action> actions: [
        Action {
            text: qsTr("Help")
            icon.source: App.faUrlBase + "question"
            onTriggered: helpDialog.visible = true
        },
        Action {
            text: qsTr("Save")
            icon.source: App.faUrlBase + "floppy-o"
            onTriggered: App.saveIgnorePatterns(page.dirId, textArea)
        }
    ]
    property list<Action> extraActions: [
        Action {
            text: qsTr("Undo")
            icon.source: App.faUrlBase + "undo"
            onTriggered: textArea.undo()
            enabled: textArea.canUndo
        },
        Action {
            text: qsTr("Redo")
            icon.source: App.faUrlBase + "repeat"
            onTriggered: textArea.redo()
            enabled: textArea.canRedo
        },
        Action {
            text: qsTr("Clear")
            icon.source: App.faUrlBase + "eraser"
            onTriggered: textArea.clear()
            enabled: textArea.length > 0
        },
        Action {
            text: qsTr("Ignore all")
            icon.source: App.faUrlBase + "filter"
            onTriggered: textArea.append((App.connection.pathSeparator || "/") + "**")
        },
        Action {
            text: qsTr("Edit externally")
            icon.source: App.faUrlBase + "external-link"
            enabled: App.connection.isLocal
            onTriggered: App.openIgnorePatterns(page.dirId)
        }
    ]
    ScrollView {
        anchors.fill: parent
        TextArea {
            id: textArea
            width: parent.width
            enabled: false
        }
    }
    BusyIndicator {
        anchors.centerIn: parent
        running: !textArea.enabled
    }
    CustomDialog {
        id: helpDialog
        title: qsTr("Quick guide to patterns")
        standardButtons: Dialog.Ok
        implicitWidth: 500
        contentItem: ScrollView {
            contentWidth: availableWidth
            Label {
                width: parent.width
                height: implicitHeight
                wrapMode: Text.Wrap
                textFormat: Text.RichText
                text: `
                    <table>
                        <tr><td><code>(?d)</code></td><td><strong>${qsTr("Prefix indicating that the file can be deleted if preventing directory removal")}</strong></td></tr>
                        <tr><td><code>(?i)</code></td><td>${qsTr("Prefix indicating that the pattern should be matched without case sensitivity")}</td></tr>
                        <tr><td><code>!</code></td><td>${qsTr("Inversion of the given condition (i.e. do not exclude)")}</td></tr>
                        <tr><td><code>*</code></td><td>${qsTr("Single level wildcard (matches within a directory only)")}</td></tr>
                        <tr><td><code>**</code></td><td>${qsTr("Multi level wildcard (matches multiple directory levels)")}</td></tr>
                        <tr><td><code>//</code></td><td>${qsTr("Comment, when used at the start of a line")}</td></tr>
                    </table>
                `
            }
        }
        footer: DialogButtonBox {
            Button {
                text: qsTr("Full documentation")
                flat: true
                onClicked: App.openUrlExternally("https://docs.syncthing.net/users/ignoring")
                DialogButtonBox.buttonRole: DialogButtonBox.HelpRole
            }
        }
    }

    required property string dirName
    required property string dirId
}
