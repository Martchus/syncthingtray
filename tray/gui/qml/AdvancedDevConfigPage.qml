import QtQuick

AdvancedConfigPage {
    title: qsTr("Advanced config of device \"%1\"").arg(devName)
    entriesKey: "devices"
    isEntry: (device) => device.deviceID === devId
    configObject: devId.length > 0 ? findConfigObject() : makeNewConfig()
    configCategory: "config-option-device"
    required property string devName
    required property string devId
    function makeNewConfig() {
        return app.connection.rawConfig?.defaults?.device ?? {};
    }
}
