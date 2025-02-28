import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

import Main

Page {
    id: page
    title: qsTr("Move Syncthing home directory")
    Layout.fillWidth: true
    Layout.fillHeight: true
    Component.onCompleted: page.load()
    CustomListView {
        id: dirsView
        anchors.fill: parent
        model: ListModel {
        }
        delegate: ItemDelegate {
            id: dirDelegate
            width: dirsView.width
            onClicked: page.setSelectedDir(dirDelegate.modelData.index)
            contentItem: RowLayout {
                RadioButton {
                    checkable: false
                    checked: dirDelegate.modelData.selected
                    onClicked: dirDelegate.onClicked()
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: dirDelegate.modelData.label
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.fillWidth: true
                        text: dirDelegate.modelData.path
                        elide: Text.ElideRight
                        font.weight: Font.Light
                        wrapMode: Text.Wrap
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: page.warningForDir(dirDelegate.modelData.populated, dirDelegate.modelData.defaultSelected)
                        elide: Text.ElideRight
                        font.weight: Font.Light
                        wrapMode: Text.Wrap
                    }
                }
            }
            required property var modelData
        }
        footer: ItemDelegate {
            id: customDirDelegate
            width: dirsView.width
            onClicked: customDirDlg.open()
            contentItem: RowLayout {
                RadioButton {
                    checkable: false
                    checked: page.customDirSelected
                    onClicked: customDirDelegate.onClicked()
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Custom path")
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.fillWidth: true
                        text: page.customDirPath.length > 0 ? page.customDirPath : qsTr("Click to select a custom path")
                        elide: Text.ElideRight
                        font.weight: Font.Light
                        wrapMode: Text.Wrap
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: page.customDirPath.length > 0 && text.length > 0
                        text: page.warningForDir(page.customDirPopulated)
                        elide: Text.ElideRight
                        font.weight: Font.Light
                        wrapMode: Text.Wrap
                    }
                }
            }
            required property var modelData
        }
    }
    FolderDialog {
        id: customDirDlg
        title: qsTr("Select custom home directory")
        onAccepted: {
            page.customDirPath = App.resolveUrl(customDirDlg.selectedFolder);
            page.setSelectedDir(-1);
        }
    }
    readonly property bool isDangerous: true
    property int currentHomePopulated: -1
    property bool customDirSelected: false
    property string customDirPath: ""
    property int customDirPopulated: customDirPath.length > 0 ? page.isPopulated(customDirPath) : -1
    property list<Action> actions: [
        Action {
            text: qsTr("Move home to selected path")
            icon.source: App.faUrlBase + "download"
            onTriggered: {
                const selectedDir = page.findSelectedDir();
                selectedDir === undefined
                        ? App.showError(qsTr("No directory selected."))
                        : App.moveSyncthingHome(selectedDir, () => page.load())
            }
        }
    ]
    function load() {
        App.checkSyncthingHome((homeInfo) => {
            const model = dirsView.model;
            model.clear();
            page.currentHomePopulated = page.populatedPropertyToInt(homeInfo.currentHomePopulated);
            page.customDirSelected = false;
            page.customDirPath = "";
            homeInfo.availableDirs.forEach((dir, index) => {
                dir.index = index;
                dir.defaultSelected = dir.selected;
                dir.populated = page.populatedPropertyToInt(dir.populated);
                model.append(dir);
            });
        });
    }
    function populatedPropertyToInt(populated) {
        return (populated === undefined || populated === null) ? (-1) : (populated ? 1 : 0);
    }
    function isPopulated(dir) {
        return page.populatedPropertyToInt(App.isPopulated(dir));
    }
    function findSelectedDir() {
        const model = dirsView.model;
        for (let i = 0, count = model.count; i !== count; ++i) {
            const modelData = model.get(i);
            if (modelData.selected) {
                return modelData.value;
            }
        }
        if (customDirSelected && customDirPath.length > 0) {
            return customDirPath;
        }
        return undefined;
    }
    function setSelectedDir(index) {
        const model = dirsView.model;
        for (let i = 0, count = model.count; i !== count; ++i) {
            model.setProperty(i, "selected", i === index);
        }
        page.customDirSelected = index < 0;
    }
    function warningForDir(populated, defaultSelected = false) {
        if (defaultSelected) {
            return qsTr("This is the current home directory.");
        } else if (populated > 0) {
            if (page.currentHomePopulated) {
                return qsTr("Warning: This directory is not empty and its contents will be replaced with the current home directory!");
            } else {
                return qsTr("Warning: This directory is not empty. Its contents will be used as new home directory.");
            }
        } else if (populated < 0) {
            return qsTr("Warning: This path can probably not be used as home directory.");
        } else {
            return "";
        }
    }
}
