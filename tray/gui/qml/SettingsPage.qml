import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

StackView {
    id: stackView
    Layout.fillWidth: true
    Layout.fillHeight: true
    initialItem: Page {
        id: appSettingsPage
        title: qsTr("App settings")
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: listView
            anchors.fill: parent
            model: ListModel {
                id: model
                ListElement {
                    key: "connection"
                    label: qsTr("Connection to Syncthing backend")
                    title: qsTr("Configure connection with Syncthing backend")
                    iconName: "link"
                }
                ListElement {
                    functionName: "importSettings"
                    label: qsTr("Import settings/data of app and backend, including secrets")
                    iconName: "download"
                }
                ListElement {
                    functionName: "exportSettings"
                    label: qsTr("Export settings/data of app and backend, including secrets")
                    iconName: "floppy-o"
                }
            }
            delegate: ItemDelegate {
                width: listView.width
                text: label
                //icon.source: app.faUrlBase + iconName // leads to crash when closing UI
                onClicked: key.length === 0 ? app[functionName]() : stackView.push("ObjectConfigPage.qml", {title: title, configObject: appSettingsPage.config[key], stackView: stackView}, StackView.PushTransition)
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        property var config: app.settings
        property list<Action> actions: [
            Action {
                text: qsTr("Apply")
                icon.source: app.faUrlBase + "check"
                onTriggered: (source) => {
                    const cfg = app.settings;
                    for (let i = 0, count = model.count; i !== count; ++i) {
                        const entryKey = model.get(i).key;
                        cfg[entryKey] = appSettingsPage.config[entryKey]
                    }
                    app.settings = cfg
                    return true;
                }
            }
        ]
    }
}
