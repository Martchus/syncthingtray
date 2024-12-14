import QtQuick

import Main

AdvancedConfigPage {
    title: qsTr("Advanced config of device \"%1\"").arg(devName)
    entryName: "device"
    entriesKey: "devices"
    isEntry: (device) => device.deviceID === devId
    configCategory: "config-option-device"
    required property string devName
    required property string devId
    function makeNewConfig() {
        const config = App.connection.rawConfig?.defaults?.device ?? {};
        if (devId.length > 0) {
            config.deviceID = devId;
        }
        isNew = true;
        return config;
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
