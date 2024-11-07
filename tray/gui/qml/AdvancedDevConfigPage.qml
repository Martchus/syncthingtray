import QtQuick

import Main

AdvancedConfigPage {
    title: qsTr("Advanced config of device \"%1\"").arg(devName)
    entryName: "device"
    entriesKey: "devices"
    isEntry: (device) => device.deviceID === devId
    configCategory: "config-option-device"
    Component.onCompleted: configObject = devId.length > 0 ? findConfigObject() : makeNewConfig();
    required property string devName
    required property string devId
    function makeNewConfig() {
        return App.connection.rawConfig?.defaults?.device ?? {};
    }
    function updateIdentification() {
        const id = configObject.deviceID ?? "";
        const name = configObject.name ?? "";
        devName = name.length > 0 ? name : id;
        if (!configObjectExists) {
            devId = id;
        }
    }
}
