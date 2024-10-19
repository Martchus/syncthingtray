import QtQuick

AdvancedConfigPage {
    title: qsTr("Advanced config of device \"%1\"").arg(devName)
    entryName: "device"
    entriesKey: "devices"
    isEntry: (device) => device.deviceID === devId
    configObject: devId.length > 0 ? findConfigObject() : makeNewConfig()
    configCategory: "config-option-device"
    required property string devName
    required property string devId
    function makeNewConfig() {
        return app.connection.rawConfig?.defaults?.device ?? {};
    }
    function updateIdentification() {
        const id = configObject.deviceID ?? "";
        const name = configObject.name ?? "";
        devName = name.length > 0 ? name : id;
        if (devId.length === 0) {
            devId = id;
        }
    }
}
