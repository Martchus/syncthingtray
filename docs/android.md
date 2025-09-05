# Using the Android app
**The Android app is still experimental.** Use it with care and create backups of your
configuration and data before trying it. Before filing any issues, be sure to read the section
"[Caveats on Android](#caveats-on-android)". When building from sources, checkout the
[Android-specific remarks](https://github.com/Martchus/cpp-utilities/blob/master/README.md#remarks-about-building-for-android)
of the `c++utilities` README.

## Compatibility
The Android app requires Android 9 or later. It is mainly tested on Android 14, 15 and 16, though.

Depending on the Android version and vendor-specific limitations you might run into permission
errors and [the app being stopped by the OS](https://dontkillmyapp.com).

For me it works well enough on a three year old average Samsung device except that the foreground
service of the app is frequently killed (just to be restarted almost immediately again). I also
successfully tested it on an older Nokia device with Android 10. So it probably works on most
phones that came out around 2018 or more recently. I haven't tested the app on very low-end
devices yet.

## General remarks
If you're starting from scratch you can simply install and start the app. Otherwise, checkout
the sections about migrating after reading the general remarks.

In any case, you need to give the app *notification permission* and *storage permission* via the
app settings of Android. The start page of the app shows "Request … permission" actions at the
top for opening the app settings if the permissions haven't been granted yet.

The app will start Syncthing automatically by default. Once Syncthing is running you can add
devices and folders as usual. The official Syncthing documentation applies. There are also many
help texts provided within the app itself. A few additional remarks:

* You can select device IDs of nearby devices from the combo box when adding a new device. So
  there's usually no need to copy & paste device IDs. If you nevertheless need to scan a QR-code
  I recommended to simply use your camera app to scan and copy the QR-code information and paste
  it into the Syncthing Tray app. So far there is no in-app QR-code scanning.
* You can leave the device name empty to use the name the device advertises. The device name can
  also be changed later (in contrast to IDs).
* If you have already another Syncthing app installed or you have an existing configuration from
  another device, read the next sections for testing/migrating.
* The app does not automatically trigger media library rescans, so e.g. synchronized music might
  not show up immediately in your music app. However, you can trigger a rescan manually if needed.
  You can do this per folder from the context menu on the folders page.
* It is highly recommended to enable the option "Ignore permissions" on all folders under Android
  and when certain file systems are used. The app enables this option therefore by default in
  such cases when a path for a new folder has been selected. You can still disable the option
  again manually.
* It is probably also a good idea to enable authentication for accessing the web-based UI as
  otherwise any other app would be able to access it. A user name and password can be configured
  under "Advanced" → "Syncthing API and web-based GUI". Alternatively, one can enable the use of
  a UNIX domain socket under "Tweaks". This also prevents other apps from accessing the
  web-based UI by making it completely inaccessible and by the way avoiding communication
  overhead.
* The app exposes its private directories via a "document provider" so you can grant other apps
  the permission to access them, e.g. to browse the Syncthing home directory containing the
  Syncthing configuration and database in a file browser that supports "document providers".
  Checkout the "[Using the document provider on Android](#using-the-document-provider-on-android)"
  for details.

## Testing the app without migrating
To only test the app without migrating your setup you can follow the steps of this section. Note
that changes done via the app *will* affect your existing Syncthing setup. You are *not* working
in read-only mode or on a copy.

0. Start the Syncthing app you are currently using (e.g. the Syncthing-Fork app) and ensure that
   Syncthing itself is running as well.
1. Lookup the listening address and API key in the options of the app you are currently using.
   Alternatively, do an export/backup in the app you are currently using. This might be a good
   idea anyway. The backup directory will contain the file `config.xml` which also contains the
   listening address and API key. The export will also contain the HTTPs certificate which you
   need if HTTPs is used.
2. Install and start the Syncthing app from Syncthing Tray.
3. Ensure that running Syncthing is disabled under the runtime condition settings.
4. Configure the information from step 1 in the connection settings. When using HTTPs the
   certificate is required.
5. After applying all settings, the app UI can be used to control your existing Syncthing setup.

## Migrating data from your existing app setup
You can follow these steps when switching apps on the same device. If you decided to import only
specific devices and folders (recommended) you can still keep using your existing app setup
(keeping both apps installed in parallel).

You can also follow these steps when setting up Syncthing on a new device based on the
configuration from another device.

0. Start the Syncthing app you are currently using (e.g. the Syncthing-Fork app) and wait until
   everything is in-sync.
1. Do an export in the app you are currently using. This will populate a directory on the internal
   or external storage.
2. Stop the Syncthing app you are currently using.
3. Start the Syncthing app from Syncthing Tray and enable running Syncthing in the app settings.
4. Import the config and data from step 1 in the app settings. When clicking on "Import …" you
   will be prompted to select the directory you stored the export made in step 1. After selecting
   the directory no changes will be made immediately; the app will show you what it found and
   allow you to and select what parts you want to import.
    * It is recommended to select only specific folders and devices. Then the Syncthing Tray app
      will keep its current device ID. This means it will appear as a new device on other devices.
      So you will have to add it on other devices as a new device and accept sharing relevant
      folders.
        * All devices and folders will be added in paused state. So you can still tweak settings
          and ignore patterns before any scanning or syncing takes place.
        * Changing folder paths is not possible after the import anymore, though. Therefore you
          have to change the paths of the folders during the import if necessary.
        * When setting up a completely new device you can simply select/create empty folders
          where you want the imported folders to be. Then Syncthing will pull the contents from
          other devices once you unpause the folders/devices.
    * It is also possible to do a full import. This will import the Syncthing configuration and
      database from the export. That means the device ID from your existing app setup will be
      reused. In fact, the entire Syncthing configuration and database will be reused. So the
      setup will be identical to how it was before - except that now a different app is used.
      That also means that no changes are required on other devices. *The big caveat of this method
      is that you should not start the other app anymore unless you re-import the config/database
      there and make sure the app from Syncthing Tray is stopped. Reusing the Syncthing database
      is also generally considered dangerious and therefore not recommended when setting up a new
      device.*

## Differences between Syncthing Tray on Android and the Syncthing-Fork app
* The [Syncthing-Fork](https://github.com/Catfriend1/syncthing-android) app is generally more
  mature/stable. Syncthing Tray has still many caveats on Android (see next section).
* The Syncthing-Fork app uses Android's native UI framework and therefore has a more native UI
  than Syncthing Tray which uses Qt. The UI of Syncthing Tray still follows the Material style
  guidelines and provides native file dialogs and notifications.
* The UI of Syncthing Tray on Android is more in-line with the UI of Syncthing Tray on the desktop
  and the official web-based UI.
* The UI of Syncthing Tray on Android allows changing all advanced settings and has built-in help
  texts for many options in accordance with the official Syncthing documentation.
* Syncthing Tray allows browsing the global file tree of a folder. Items can be selected and added
  to ignore patterns.
* The Syncthing-Fork app provides many features that haven't been implemented yet by Syncthing Tray,
  e.g. advanced run conditions. Syncthing Tray allows stopping Syncthing on metered network
  connections, though.
* The Syncthing-Fork app works probably better on older or very low-budget devices.

## Caveats on Android
While Syncthing Tray basically works on Android, there are still some unresolved issues:

* The Go runtime and thus the service process sometimes "panics" which still needs debugging. It
  is restarted by Android automatically, though. (Just like it is restarted after being sometimes
  killed forcefully by the Android.)
* The performance can be problematic due to the use of FUSE as of Android 11. Especially if one
  has many files in one directory the performance is bad.
    * I recommended to avoid having many files in a single directory.
    * Alternatively one can store the data in one of the app's private directories (on the internal
      storage or the SD card) which can be faster due to
      [FUSE Passthrough](https://source.android.com/docs/core/storage/fuse-passthrough). To be able
      to do this, the app exposes the private directories as "document provider" which are this way
      selectable via the Android file selection dialog from other apps. This of course does not
      cover all use cases as other apps might only be able to use files from fixed directories.
      Checkout the section
      "[Using the document provider on Android](#using-the-document-provider-on-android)" for details.
* Media rescans need to be triggered manually but this can be easily done per folder from the UI.
* There are probably still many small UI bugs in the Qt Quick based UI used on Android.
* The Syncthing home directory needs to be within the private directory of the app on the main
  storage. The app allows moving the home directory to other locations, e.g. the private directory
  of the app on the SD card. However, Syncthing fails to open its database on other locations. (Having
  the database on the SD card seems to work with Syncthing v2 which switched to SQLite. So this caveat
  will likely be removed when Syncthing v2 is released.)
* Not all features the official web UI offers have been implemented in the Qt Quick based UI yet.
  One can easily open the official web UI in a web browser, though.
* Some of the problems/solutions found on the
  [Wiki pages of Syncthing-Fork](https://github.com/Catfriend1/syncthing-android/wiki) might help with
  Syncthing Tray on Android as well.

## Using the document provider on Android
Syncthing Tray provides a
[document provider](https://developer.android.com/reference/android/provider/DocumentsProvider) under
Android which allows other apps (e.g. file managers) to access private directories of Syncthing and
Syncthing Tray. Other apps have of course to ask for permission before accessing the exposed
directories.

The following directories are exposed:

* Home directory: This directory contains the settings of the app/wrapper (`settings/appconfig.json`)
  and the Syncthing config file and database (`settings/syncthing/…`). Accessing this directory
  is mainly useful for debugging and troubleshooting.
* Internal and external storage: These directories are not used by the app/wrapper or Syncthing by
  default.
    * You may move the Syncthing config and database there in the app settings. This can be useful
      to free space on the internal storage by moving the possibly big Syncthing database to the
      SD card. (This may have performance implications and you may run into other limitations.)
    * You may create Syncthing folders there. This can be useful if other apps are not supposed to
      access the contents of these folders without asking for permission first. Doing this may also
      help to workaround performance limitations, checkout the
      "[Caveats on Android](#caveats-on-android)" section for details.

In order to access files from the exposed directories you need to trigger the standard file dialog
from Android in the app you want to open the file with. In the standard file dialog the directories
are selectable from the left drawer.

Unfortunately not all file managers support browsing directories exposed via a document provider:

* The standard "Files" app from Android supports it. One can select the exposed directories from
  the left drawer. Unfortunately this app is often replaced with a different app by device vendors,
  e.g. the app is not present on the Samsung devices I tested on.
* The open source app [Material Files](https://github.com/zhanghai/MaterialFiles) supports it as
  well and can be used as alternative if "Files" is not present. One can add the exposed
  directories via "Add storage → External storage" and then select them from the left drawer.
* The "Files by Google" app does *not* support it. The app recognizes the document provider but
  only opens the app itself instead of letting one manage the files.
* The "My Files" app present on Samsung devices does not seem to support custom document providers
  at all.
