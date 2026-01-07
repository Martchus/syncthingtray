# Using the Android app
**The Android app is still experimental.** Use it with care and create backups of your
configuration and data before trying it. Before filing any issues, be sure to read the section
"[Caveats on Android](#caveats-on-android)". For screenshots, check out the section
"[Screenshots of the mobile UI](screenshots.md#mobile-ui-on-android)".

The app can be installed and updated manually or via [Obtainium](https://obtainium.imranr.dev/).
Check out [the website](https://martchus.github.io/syncthingtray/#downloads-section)
for download links. When building from sources, check out the
[Android-specific remarks](https://github.com/Martchus/cpp-utilities/blob/master/README.md#remarks-about-building-for-android)
of the `c++utilities` README.

## Compatibility
The Android app requires Android 9 or later, though it is mainly tested on Android 14, 15 and 16.

Depending on the Android version and vendor-specific limitations, you might run into permission
errors and [the app being stopped by the OS](https://dontkillmyapp.com).

For me, it works well enough on a three-year-old average Samsung device. I also successfully
tested it on an older Nokia device with Android 10. So, it probably works on most phones that
came out around 2018 or more recently. I haven't tested the app on very low-end devices yet.

## Getting started
The first things to do after installing and starting the app:

* You need to give the app *notification permission* and *storage permission* via the Android app
  settings. The start page of the app shows "Request … permission" actions at the top for opening
  the app settings if the permissions haven't been granted yet.
* The app will start Syncthing automatically by default. You can also disable this and configure
  it to connect with an externally launched instance of Syncthing.
* It is a good idea to enable authentication for accessing the web-based UI, as otherwise, any
  other app would be able to access it. A username and password can be configured under
  "Advanced → Syncthing API and web-based GUI". Alternatively, you can enable the use of a UNIX
  domain socket under "Tweaks". This also prevents other apps from accessing the web-based UI by
  making it completely inaccessible and, by the way, avoiding communication overhead.
* If you already have another Syncthing app installed or you have an existing configuration from
  another device, read the next sections for testing/migrating.
* You can add devices and folders as usual. The official Syncthing documentation applies. There
  are also many help texts provided within the app itself. A few additional remarks:
  * You can select device IDs of nearby devices from the combo box when adding a new device. So,
    there's usually no need to copy & paste device IDs. If you nevertheless need to scan a QR code,
    I recommend simply using your camera app to scan and copy the QR code information and paste
    it into the Syncthing Tray app. So far, there is no in-app QR code scanning.
  * You can leave the device name empty to use the name the device advertises. The device name can
    also be changed later (in contrast to the IDs).
  * It is highly recommended to enable the option "Ignore permissions" on all folders under Android
    and when certain file systems are used. The app, therefore, enables this option by default in
    such cases when a path for a new folder has been selected. You can still disable the option
    manually.

## Important remarks
* The app does not automatically trigger media library rescans, so, e.g., synchronized music might
  not show up immediately in your music app. However, you can trigger a rescan manually if needed.
  You can do this per folder from the context menu on the folders page.
* The app exposes its private directories via a "document provider" so you can grant other apps
  permission to access them, e.g., to browse the Syncthing home directory containing the
  Syncthing configuration and database in a file browser that supports "document providers".
  Check out the "[Using the document provider on Android](#using-the-document-provider-on-android)"
  for details. Subdirectories within the private directories can also be added as Syncthing
  folders.

## Synchronizing only some files
If you don't want to synchronize a folder completely it makes sense to configure ignore patterns.
To make things a little bit easier, the app features a "Remote files" browser that allows one to
manage ignore patterns. To use this, one would:

0. Read the [documentation on ignore patterns](https://docs.syncthing.net/users/ignoring.html).
1. Add a new folder keeping the setting "Paused" enabled to prevent Syncthing from immediately
    pulling files.
2. Add `/**` as ignore pattern to ignore everything by default.
3. Resume the folder and wait until it is idle.
4. Open the "Remote files" browser for the folder and select directories and files to synchronize.
5. Review the changes to the ignore pattern and apply them if everything looks right. In this
   last step you can also do manual changes, e.g. replacing certain path elements with globbing.

## Testing the app without migrating
To only test the app without migrating your setup, you can follow the steps of this section. Note
that changes done via the app *will* affect your existing Syncthing setup. You are *not* working
in read-only mode or on a copy.

0. Start the Syncthing app you are currently using (e.g. the Syncthing-Fork app) and ensure that
   Syncthing itself is running as well.
1. Lookup the listening address and API key in the options of the app you are currently using.
   Alternatively, do an export/backup in the app you are currently using. This might be a good
   idea anyway. The backup directory will contain the file `config.xml`, which also contains the
   listening address and API key. The export will also contain the HTTPS certificate, which you
   need if HTTPS is used.
2. Install and start the Syncthing app from Syncthing Tray.
3. Ensure that running Syncthing is disabled under the runtime condition settings.
4. Configure the information from step 1 in the connection settings. When using HTTPS, the
   certificate is required.
5. After applying all settings, the app UI can be used to control your existing Syncthing setup.

## Migrating data from your existing app setup
You can follow these steps when switching apps on the same device. If you decide to import only
specific devices and folders (recommended), you can still keep using your existing app setup
(keeping both apps installed in parallel).

You can also follow these steps when setting up Syncthing on a new device based on the
configuration from another device.

0. Start the Syncthing app you are currently using (e.g. the Syncthing-Fork app) and wait until
   everything is in-sync.
1. Do an export in the app you are currently using. This will populate a directory on the internal
   or external storage.
2. Stop the Syncthing app you are currently using.
3. Start the Syncthing app from Syncthing Tray and enable running Syncthing in the app settings.
4. If the export done in step 1 created an archive, you need to enable "Import/export archive"
   under "App settings → Tweaks". If the archive is encrypted, you'll also need to enter the
   password there. Only Zip archives are supported. If a different archiving format is used,
   you'll have to extract the archive first and make sure that Import/export archive" is *not*
   enabled.
5. Import the config and data from step 1 in via "App settings → Import …". You will be prompted
   to select the directory or archive that was created by the export in step 1. After the selection,
   no changes will be made immediately; the app will show you what it found and allow you to select
   what parts you want to import.
    * It is recommended to select only specific folders and devices. Then, the Syncthing Tray app
      will keep its current device ID. This means it will appear as a new device on other devices.
      So, you will have to add it on other devices as a new device and accept sharing relevant
      folders.
        * All devices and folders will be added in a paused state. So, you can still tweak settings
          and ignore patterns before any scanning or syncing takes place.
        * Changing folder paths is not possible after the import, though. Therefore, you
          have to change the paths of the folders during the import if necessary.
        * When setting up a completely new device, you can simply select/create empty folders
          where you want the imported folders to be. Then, Syncthing will pull the contents from
          other devices once you unpause the folders/devices.
    * It is also possible to do a full import. This will import the Syncthing configuration and
      database from the export. That means the device ID from your existing app setup will be
      reused. In fact, the entire Syncthing configuration and database will be reused. So, the
      setup will be identical to how it was before — except that now a different app is used.
      That also means that no changes are required on other devices. *The big caveat of this method
      is that you should not start the other app anymore unless you re-import the config/database
      there and ensure the app from Syncthing Tray is stopped. Reusing the Syncthing database
      is also generally considered dangerous and therefore not recommended when setting up a new
      device.*

## Differences between Syncthing Tray on Android and the Syncthing-Fork app by Catfriend1
The following section describes differences between Syncthing Tray and the
[Syncthing-Fork app](https://github.com/Catfriend1/syncthing-android) which used to be
maintained by Catfriend1. **That this app is now maintained by someone else which is
[discussed on the Syncthing forums](https://forum.syncthing.net/t/does-anyone-know-why-syncthing-fork-is-no-longer-available-on-github).**
This section is *not* taking changes after the maintainer switch into account.

* The Syncthing-Fork app is generally more mature/stable than Syncthing Tray which still has
  many caveats on Android (see next section).
* The Syncthing-Fork app uses Android's native UI framework and therefore has a more native UI
  than Syncthing Tray, which uses Qt. The UI of Syncthing Tray still follows the Material style
  guidelines and provides native file dialogs and notifications.
* The UI of Syncthing Tray on Android is more in line with the UI of Syncthing Tray on the desktop
  and the official web-based UI.
* The UI of Syncthing Tray on Android allows changing all advanced settings and has built-in help
  texts for many options in accordance with the official Syncthing documentation.
* Syncthing Tray allows browsing the global file tree of a folder. Items can be selected and added
  to ignore patterns.
* The Syncthing-Fork app provides many features that haven't been implemented yet by Syncthing Tray,
  e.g., advanced run conditions. Syncthing Tray allows stopping Syncthing on metered network
  connections, however.
* The Syncthing-Fork app probably works better on older or very low-budget devices.

## Caveats on Android
While Syncthing Tray basically works on Android, there are still some unresolved issues:

* The performance can be problematic due to the use of FUSE as of Android 11. Especially if you
  have many files in one directory, the performance is bad.
    * I recommend avoiding having many files in a single directory.
    * Alternatively, you can store the data in one of the app's private directories (on the internal
      storage or the SD card), which can be faster due to
      [FUSE Passthrough](https://source.android.com/docs/core/storage/fuse-passthrough). To be able
      to do this, the app exposes the private directories as a "document provider," which are this way
      selectable via the Android file selection dialog from other apps. This, of course, does not
      cover all use cases, as other apps might only be able to use files from fixed directories.
      Check out the section
      "[Using the document provider on Android](#using-the-document-provider-on-android)" for details.
* Media rescans need to be triggered manually, but this can be easily done per folder from the UI.
* There are probably still many small UI bugs in the Qt Quick based UI used on Android.
* Not all features the official web UI offers have been implemented in the Qt Quick based UI yet.
    * Most notably, there is no UI for restoring old versions. (You can configure versioning, though.)
    * As a workaround, you can open the official web UI in a web browser.
* When switching back from another app, the state of the UI might be lost. This is especially annoying
  when adding a new folder or device. This can also happen when opening the Syncthing documentation
  from within the app. So if you are on a device with low memory (which makes this more likely to
  happen), you should avoid switching to another app or opening the Syncthing documentation.
* The app is not always able to suppress connection errors when the Syncthing backend is restarted.
* The app can display an Android notification when the service process encounters an error while
  communicating with the Syncthing backend. These error notifications do not necessarily correspond to
  the errors shown in the app UI, as those originate from the UI process.
* The connection to Syncthing can sometimes not be restored after restarting Syncthing (e.g. to make
  an export/backup). This happens particularly often when using a UNIX domain socket. One can restart
  the app to work around it.
* Battery-life can be a problem. If you are affected I recommend disabling local discovery. Note that
  this is a problem of Syncthing itself and there is already
  [a discussion in the forums](https://forum.syncthing.net/t/syncthing-fork-v2-uses-far-too-much-battery-over-v1)
  and [a potential workaround](https://github.com/syncthing/syncthing/pull/10494).
* It is possible to pause Syncthing if the network connection is metered. There are two options for
  this. The option in the launcher settings completely stops/starts Syncthing depending on the network
  connection. With
  [rescans on startup not being optional](https://github.com/syncthing/syncthing/issues/5353) this will
  drain the battery if you have big folders. The option in the connection settings will keep Syncthing
  running but pause devices, discovery and relaying. This will hopefully not drain the battery as much
  but it can lead to inconsistencies, e.g. if you add a new device while the connection is metered.
* Some of the problems/solutions found on the
  [Wiki pages of Syncthing-Fork](https://github.com/Catfriend1/syncthing-android/wiki) might help with
  Syncthing Tray on Android as well.
* Some Qt bugs mentioned under "[List of bugs](known_bugs_and_workarounds.md#list-of-bugs)" affect
  the Android app.

## Using the document provider on Android
Syncthing Tray provides a
[document provider](https://developer.android.com/reference/android/provider/DocumentsProvider) under
Android which allows other apps (e.g., file managers) to access private directories of Syncthing and
Syncthing Tray. Other apps, of course, have to ask for permission before accessing the exposed
directories.

The following directories are exposed:

* Home directory: This directory contains the settings of the app/wrapper (`settings/appconfig.json`)
  and the Syncthing config file and database (`settings/syncthing/…`). Accessing this directory
  is mainly useful for debugging and troubleshooting.
* Internal and external storage: These directories are not used by the app/wrapper or Syncthing by
  default.
    * You may move the Syncthing config and database there in the app settings. This can be useful
      to free up space on the internal storage by moving the possibly large Syncthing database to the
      SD card. (This may have performance implications, and you may run into other limitations.)
    * You may create Syncthing folders there. This can be useful if other apps are not supposed to
      access the contents of these folders without asking for permission first. Doing this may also
      help to work around performance limitations; check out the
      "[Caveats on Android](#caveats-on-android)" section for details.

In order to access files from the exposed directories, you need to trigger the standard file dialog
from Android in the app you want to open the file with. In the standard file dialog, the directories
are selectable from the left drawer.

Unfortunately, not all file managers support browsing directories exposed via a document provider:

* The standard "Files" app from Android supports it. You can select the exposed directories from
  the left drawer. Unfortunately, this app is often replaced with a different app by device vendors,
  e.g., the app is not present on the Samsung devices I tested on.
* The open-source app [Material Files](https://github.com/zhanghai/MaterialFiles) also supports it
  and can be used as an alternative if "Files" is not present. You can add the exposed
  directories via "Add storage → External storage" and then select them from the left drawer.
* The "Files by Google" app does *not* support it. The app recognizes the document provider but
  only opens the app itself instead of letting you manage the files.
* The "My Files" app present on Samsung devices does not seem to support custom document providers
  at all.
