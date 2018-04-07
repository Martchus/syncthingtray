function findFolder(id) {
    var folders = config.folders;
    for (var i = 0, count = folders.length; i !== count; ++i) {
        var folder = folders[i];
        if (folder.id === id) {
            return folder;
        }
    }
}

function findDevice(name) {
    var devices = config.devices;
    for (var i = 0, count = devices.length; i !== count; ++i) {
        var device = devices[i];
        if (device.name === name) {
            return device;
        }
    }
}

function findDeviceById(id) {
    var devices = config.devices;
    for (var i = 0, count = devices.length; i !== count; ++i) {
        var device = devices[i];
        if (device.deviceID === name) {
            return device;
        }
    }
}

function assignIfPresent(object, property, value) {
    if (!object) {
        return;
    }
    object[property] = value;
}
