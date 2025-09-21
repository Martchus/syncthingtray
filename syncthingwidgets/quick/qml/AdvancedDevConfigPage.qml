import QtQuick

import Main

AdvancedConfigPage {
    title: qsTr("Advanced config of device \"%1\"").arg(devName)
    entryName: qsTr("device")
    entriesKey: "devices"
    isEntry: (device) => device.deviceID === devId
    configCategory: "config-option-device"
    required property string devName
    required property string devId
    function makeNewConfig() {
        const config = App.connection.rawConfig?.defaults?.device ?? {};

        // add device ID and name as default values for deviceID/name
        if (devId.length > 0) {
            config.deviceID = devId;
        }
        if (devName.length > 0) {
            config.name = devName;
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
