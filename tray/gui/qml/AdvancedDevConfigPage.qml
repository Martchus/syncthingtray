import QtQuick

AdvancedConfigPage {
    title: qsTr("Advanced config of device \"%1\"").arg(devName)
    entriesKey: "devices"
    isEntry: (device) => device.deviceID === devId
    configCategory: "config-option-device"
    required property string devName
    required property string devId
}
