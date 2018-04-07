// example script for changing configuration with syncthingctl
// can be executed like: syncthingctl edit --script example.js

// the ECMAScript environment is either provided by Qt QML or Qt Script, see http://doc.qt.io/qt-5/qtqml-javascript-hostenvironment.html
// additional helpers are defined in syncthingtray/cli/resources/js/helper.js

// alter some options
config.gui.useTLS = true;
config.options.relaysEnabled = false;

// enable file system watcher for all folders starting with "docs-"
var folders = config.folders;
for (var i = 0, count = folders.length; i !== count; ++i) {
    var folder = folders[i];
    if (folder.id.indexOf("docs-") === 0) {
        folder.fsWatcherDelayS = 50;
        folder.fsWatcherEnabled = true;
        console.log("enabling file system watcher for folder " + folder.id);
    }
}

// ensure all devices are enabled
var devices = config.devices;
for (var i = 0, count = devices.length; i !== count; ++i) {
    var device = devices[i];
    if (device.paused) {
        device.paused = false;
        console.log("unpausing device " + (device.name ? device.name : device.deviceID));
    }
}

// pause folder "foo" if the folder exist
assignIfPresent(findFolder("foo"), "paused", true);
