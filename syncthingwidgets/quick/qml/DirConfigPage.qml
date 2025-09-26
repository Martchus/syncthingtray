import QtQuick

import Main

AdvancedDirConfigPage {
    id: dirConfigPage
    title: existing && dirName.length > 0 ? qsTr("Config of folder \"%1\"").arg(dirName) : qsTr("Add new folder")
    isDangerous: false
    specialEntriesOnly: true
    specialEntries: [
        {key: "id", label: qsTr("ID"), enabled: !dirConfigPage.configObjectExists, desc: qsTr("Required identifier for the folder. Must be the same on all cluster devices.")},
        {key: "label", label: qsTr("Label"), desc: qsTr("Optional descriptive label for the folder. Can be different on each device.")},
        {key: "paused", label: qsTr("Paused"), desc: qsTr("Whether this folder is (temporarily) suspended.")},
        {key: "path", label: qsTr("Path"), enabled: !dirConfigPage.configObjectExists, type: "folderpath", desc: qsTr("Path to the folder on the local computer. Will be created if it does not exist. The tilde character (~) can be used as a shortcut for \"%1\".").arg(App.connection.tilde)},
        {key: "type", label: qsTr("Type"), type: "options", desc: qsTr("Controls how the folder is handled by Syncthing. Open the selection and go though the different options for details about them."), helpUrl: "https://docs.syncthing.net/users/foldertypes", options: [
                {value: "sendreceive", label: "Send and Receive", desc: qsTr("Files are synchronized from the cluster and changes made on this device will be sent to the rest of the cluster.")},
                {value: "sendonly", label: "Send Only", desc: qsTr("Files are protected from changes made on other devices, but changes made on this device will be sent to the rest of the cluster.")},
                {value: "receiveonly", label: "Receive Only", desc: qsTr("Files are synchronized from the cluster, but any changes made locally will not be sent to other devices.")},
                {value: "receiveencrypted", label: "Receive Encrypted", desc: qsTr("Stores and syncs only encrypted data. Folders on all connected devices need to be set up with the same password or be of type \"Receive Encrypted\" too. Can only be assigned to new folders.")},
            ]},
        {key: "devices", type: "devices", label: qsTr("Share with"), desc: qsTr("Select devices to share this folder with."), selectIds: dirConfigPage.shareWithDeviceIds},
        {key: "versioning", label: qsTr("Versioning"), desc: qsTr("Syncthing supports archiving the old version of a file when it is deleted or replaced with a newer version from the cluster. Versioning applies to changes received from <i>other</i> devices."), helpUrl: "https://docs.syncthing.net/users/versioning"},
        {key: "fsWatcherEnabled", label: qsTr("Watch for Changes"), desc: qsTr("Use notifications from the filesystem to detect changed items. Watching for changes discovers most changes without periodic scanning."), helpUrl: "https://docs.syncthing.net/users/syncing#scanning"},
        {key: "rescanIntervalS", label: qsTr("Rescan Interval"), desc: qsTr("The frequency in which Syncthing will rescan the folder for changes. Can be set to 0 to rely on triggering rescans manually.")},
        {key: "fsWatcherDelayS", label: qsTr("Watcher Delay"), desc: qsTr("The duration during which changes detected are accumulated, before a scan is scheduled. Takes only effect if \"Watch for Changes\" is enabled.")},
        {key: "fsWatcherTimeoutS", label: qsTr("Watcher Timeout"), desc: qsTr("The maximum delay before a scan is triggered when a file is continuously changing. If unset or zero a default value is calculated based on \"Watcher Delay\".")},
        {key: "minDiskFree", label: qsTr("Minimum Free Disk Space"), desc: qsTr("The minimum required free space that should be available on the disk this folder resides. The folder will be stopped when the value drops below the threshold. Set to zero to disable.")},
        {key: "order", label: qsTr("File Pull Order"), type: "options", desc: qsTr("The order in which needed files should be pulled from the cluster. It has no effect when the folder type is “send only”. Open the selection and go though the different options for details about them."), options: [
                {value: "random", label: qsTr("Random"), desc: qsTr("Pull files in random order. This optimizes for balancing resources among the devices in a cluster.")},
                {value: "alphabetic", label: qsTr("Alphabetic"), desc: qsTr("Pull files ordered by file name alphabetically.")},
                {value: "smallestFirst", label: qsTr("Smallest First"), desc: qsTr("Pull files ordered by file size; smallest first.")},
                {value: "largestFirst", label: qsTr("Largest First"), desc: qsTr("Pull files ordered by file size; largest first.")},
                {value: "oldestFirst", label: qsTr("Oldest First"), desc: qsTr("Pull files ordered by modification time; oldest first.")},
                {value: "newestFirst", label: qsTr("Newest First"), desc: qsTr("Pull files ordered by modification time; newest first.")},
            ]},
        {key: "blockPullOrder", label: qsTr("Block Pull Order"), type: "options", desc: qsTr("Order in which the blocks of a file are downloaded. This option controls how quickly different parts of the file spread between the connected devices, at the cost of causing strain on the storage. Open the selection and go though the different options for details about them."), options: [
                {value: "standard", label: qsTr("Standard"), desc: qsTr("The blocks of a file are split into N equal continuous sequences, where N is the number of connected devices. Each device starts downloading its own sequence, after which it picks other devices sequences at random. Provides acceptable data distribution and minimal spinning disk strain.")},
                {value: "random", label: qsTr("Random"), desc: qsTr("The blocks of a file are downloaded in a random order. Provides great data distribution, but very taxing on spinning disk drives.")},
                {value: "inOrder", label: qsTr("In-Order"), desc: qsTr("The blocks of a file are downloaded sequentially, from start to finish. Spinning disk drive friendly, but provides no improvements to data distribution.")},
            ]},
        {key: "ignorePerms", label: qsTr("Ignore Permissions"), desc: qsTr("Disables comparing and syncing file permissions. Useful on systems with nonexistent or custom permissions (e.g. FAT, exFAT, Synology, Android).")},
        {key: "syncOwnership", label: qsTr("Sync Ownership"), desc: qsTr("Enables sending ownership information to other devices, and applying incoming ownership information. Typically requires running with elevated privileges."), helpUrl: "https://docs.syncthing.net/advanced/folder-sync-ownership"},
        {key: "sendOwnership", label: qsTr("Send Ownership"), desc: qsTr("Enables sending ownership information to other devices, but not applying incoming ownership information. This can have a significant performance impact. Always enabled when \"Sync Ownership\" is enabled."), helpUrl: "https://docs.syncthing.net/advanced/folder-send-ownership"},
        {key: "syncXattrs", label: qsTr("Sync Extended Attributes"), desc: qsTr("Enables sending extended attributes to other devices, and applying incoming extended attributes. May require running with elevated privileges."), helpUrl: "https://docs.syncthing.net/advanced/folder-sync-xattrs"},
        {key: "sendXattrs", label: qsTr("Send Extended Attributes"), desc: qsTr("Enables sending extended attributes to other devices, but not applying incoming extended attributes. This can have a significant performance impact. Always enabled when \"Sync Extended Attributes\" is enabled."), helpUrl: "https://docs.syncthing.net/advanced/folder-sync-xattrs"},
    ]
    specialEntriesByKey: ({
        "versioning": [
            {key: "type", type: "options", desc: qsTr("There are different <i>versioning strategies</i> to choose from. Open the selection and go through the options for details on the individual versioning strategies."), helpUrl: "https://docs.syncthing.net/users/versioning", options: [
                {value: "", label: "No File Versioning", desc: qsTr("File versioning is not going to be used.")},
                {value: "trashcan", label: "Trash Can File Versioning", desc: qsTr("Files are moved to .stversions directory when replaced or deleted by Syncthing.")},
                {value: "simple", label: "Simple File Versioning", desc: qsTr("Files are moved to date stamped versions in a .stversions directory when replaced or deleted by Syncthing.")},
                {value: "staggered", label: "Staggered File Versioning", desc: qsTr("<p>Files are moved to date stamped versions in a .stversions directory when replaced or deleted by Syncthing. Versions are automatically deleted if they are older than the maximum age or exceed the number of files allowed in an interval.</p><p>The following intervals are used: for the first hour a version is kept every 30 seconds, for the first day a version is kept every hour, for the first 30 days a version is kept every day, until the maximum age a version is kept every week.</p>")},
                {value: "external", label: "External File Versioning", desc: qsTr("An external command handles the versioning. It has to remove the file from the shared folder. If the path to the application contains spaces, it should be quoted.")},
            ]},
            {key: "fsPath", label: qsTr("Versions Path"), type: "folderpath", desc: qsTr("Path where versions should be stored (leave empty for the default .stversions directory in the shared folder)."), helpUrl: "https://docs.syncthing.net/users/versioning#config-option-folder.versioning.fspath"},
            {key: "fsType", label: qsTr("Filesystem Type"), type: "string", desc: qsTr("The internal file system implementation used to access this versions folder."), helpUrl: "https://docs.syncthing.net/users/versioning#config-option-folder.versioning.fstype"},
            {key: "cleanupIntervalS", label: qsTr("Cleanup Interval in seconds"), type: "number", desc: qsTr("The interval, in seconds, for running cleanup in the versions directory. Zero to disable periodic cleaning."), helpUrl: "https://docs.syncthing.net/users/versioning#config-option-folder.versioning.fstype"},
            {key: "params", label: qsTr("Additional parameters"), type: "object"},
        ],
        "versioning.params": [
            {key: "cleanoutDays", label: qsTr("Clean Out After"), type: "string", defaultValue: 0, cond: (cop) => ["trashcan", "simple"].includes(cop?.parentObject.type ?? ""), desc: qsTr("The number of days to keep files in the versions folder. Zero means to keep forever. Older elements encountered during cleanup are removed."), helpUrl: "https://docs.syncthing.net/users/versioning#simple-file-versioning"},
            {key: "keep", label: qsTr("Keep Versions"), type: "string", defaultValue: 5, cond: (cop) => ["simple"].includes(cop?.parentObject.type), desc: qsTr("The number of old versions to keep, per file."), helpUrl: "https://docs.syncthing.net/users/versioning#simple-file-versioning"},
            {key: "maxAge", label: qsTr("Maximum Age"), type: "string", defaultValue: 0, cond: (cop) => ["staggered"].includes(cop?.parentObject.type), desc: qsTr("The maximum time to keep a version, in seconds. Zero means to keep forever."), helpUrl: "https://docs.syncthing.net/users/versioning#staggered-file-versioning"},
            {key: "command", label: qsTr("Command"), type: "string", defaultValue: "", cond: (cop) => ["external"].includes(cop?.parentObject.type), desc: qsTr("External command to execute for storing a file version about to be replaced or deleted. If the path to the application contains spaces, it should be quoted."), helpUrl: "https://docs.syncthing.net/users/versioning#external-file-versioning"},
        ],
        "minDiskFree": [
            {key: "value", label: qsTr("Value"), desc: qsTr("The minimum required free space that should be available on the disk this folder resides. The folder will be stopped when the value drops below the threshold. The value is interpreted according to the selected unit and can be set to zero to disable the check for minimum free space.")},
            {key: "unit", label: qsTr("Unit"), type: "options", options: [
                {value: "%", label: qsTr("Percent"), desc: qsTr("Percentage of the disk/volume size")},
                {value: "kB", label: qsTr("Kilobyte"), desc: qsTr("Absolute size in Kilobyte")},
                {value: "MB", label: qsTr("Megabyte"), desc: qsTr("Absolute size in Megabyte")},
                {value: "GB", label: qsTr("Gigabyte"), desc: qsTr("Absolute size in Gigabyte")},
                {value: "TB", label: qsTr("Terrabyte"), desc: qsTr("Absolute size in Terrabyte")},
            ]},
        ],
    })
    property list<string> shareWithDeviceIds
    property bool existing: true
}
