# Syncthing Tray
Syncthing Tray provides a tray icon and further platform integrations for
[Syncthing](https://github.com/syncthing/syncthing). Checkout the
[website](https://martchus.github.io/syncthingtray) for an overview and
screenshots.

The following integrations are provided:

* Tray application (using the Qt framework)
* Context menu extension for the [Dolphin](https://www.kde.org/applications/system/dolphin) file manager
* Plasmoid for [KDE Plasma](https://www.kde.org/plasma-desktop)
* Command-line interface
* Qt-ish C++ library

---

Checkout the [official forum thread](https://forum.syncthing.net/t/yet-another-syncthing-tray) for discussions
and announcement of new features.

This README document currently serves as the only and main documentation. So read on for details about
the configuration. If you are not familiar with Syncthing itself already you should also have a look at
the [Syncthing documentation](https://docs.syncthing.net) as this README is only going to cover the
Syncthing Tray integration.

Issues can be created on GitHub but please read the "[Known bugs and workarounds](#known-bugs-and-workarounds)" section in this document
before.

Syncthing Tray works with Syncthing v1 (and probably v0). Note that Syncthing Tray is maintained, and
updates will be made to support future Syncthing versions as needed.

## Supported platforms
Official binaries are provided for Windows (for i686, x86_64 and aarch64) and GNU/Linux (for x86_64) and can be
download from the [website](https://martchus.github.io/syncthingtray/#downloads-section) and the
[release section on GitHub](https://github.com/Martchus/syncthingtray/releases). This is only a fraction of
the available downloads, though. I also provide further repositories for some GNU/Linux distributions. There are
also binaries/repositories provided by other distributors. For a list with links, checkout the
"[Download](#Download)" section of this document.

Syncthing Tray is known to work under:

* Windows 10 and 11
* KDE Plasma
* Openbox using lxqt/LXDE or using Tint2
* GTK-centered desktops such as Cinnamon, GNOME and Xfce (with caveats, see remarks below)
* COSMIC (only simple tray menu works, see remarks below)
* Awesome
* i3
* macOS
* Deepin Desktop Environment
* Sway/Swaybar/Waybar (with caveats, see remarks below)
* Android (still experimental and in initial development)

This does *not* mean Syncthing Tray is actively tested on all those platforms or
desktop environments.

For Plasma 5 and 6, there is in addition to the Qt Widgets based version also a "native"
Plasmoid. Note that the latest version of the Plasmoid generally also requires the
latest version of Plasma 5 or 6 as no testing on earlier versions is done. Use the Qt
Widgets based version on other Plasma versions. Checkout the
"[Configuring Plasmoid](#configuring-plasmoid)" section for further details.

On GTK-centered desktops have a look at the
[Arch Wiki](https://wiki.archlinux.org/title/Uniform_look_for_Qt_and_GTK_applications)
for how to achieve a more native look and feel. Under GNOME one needs to install
[an extension](https://github.com/ubuntu/gnome-shell-extension-appindicator) for tray icon support (unless
one's distribution already provides such an extension by default).

Limitations of your system tray might affect Syncthing Tray. For instance when using the mentioned GNOME
extension the Syncthing Tray UI shown in the [screenshots](screenshots.md) is only shown by *double*-clicking
the icon. If your system tray is unable to show the Syncthing Tray UI at all like on COSMIC you can still use
Syncthing Tray for the tray icon and basic functionality accessible via the menu.

Note that under Wayland-based desktops there will be positioning issues. The Plasmoid is not affected
by this, though.

The section "[Known bugs and workarounds](#known-bugs-and-workarounds)" below contains further information
and workarounds for certain caveats like the positioning issues under Wayland.

## Features
* Provides quick access to most frequently used features but does not intend to replace the official web-based UI
    * Check state of folders and devices
    * Check current traffic statistics
    * Display further details about folders and devices, like last file, last
      scan, items out of sync, ...
    * Display ongoing downloads
    * Display Syncthing log
    * Trigger re-scan of a specific folder or all folders at once
    * Open a folder with the default file browser
    * Pause/resume a specific device or all devices at once
    * Pause/resume a specific folder
    * View recent history of changes (done locally and remotely)
* Shows "desktop" notifications
    * The events to show notifications for can be configured
    * Uses Qt's notification support or a D-Bus notification daemon directly
* Provides a wizard for a quick setup
* Allows monitoring the status of the Syncthing systemd unit and to start and stop it (see section
  "[Configuring systemd integration](#configuring-systemd-integration)")
* Provides an option to conveniently add the tray to the applications launched when the desktop environment starts
* Can launch Syncthing automatically when started and display stdout/stderr (useful under Windows)
* Browsing the global file tree and selecting items to add to ignore patterns.
* Provides quick access to the official web-based UI
    * Can be opened as regular browser tab
    * Can be opened in a dedicated window utilizing either
        * Qt WebEngine/WebKit
        * the "app mode" of a Chromium-based browser (e.g. Chrome and Edge)
* Allows switching quickly between multiple Syncthing instances
* Also features a simple command line utility `syncthingctl`
    * Check status
    * Trigger rescan/pause/resume/restart
    * Wait for idle
    * View and modify raw configuration
    * Supports Bash completion, even for folder and device names
* Also bundles a KIO plugin which shows the status of a Syncthing folder and allows to trigger Syncthing actions
  in the Dolphin file manager
    * Rescan selected items
    * Rescan entire Syncthing folder
    * Pause/resume Syncthing folder
    * See also the [screenshots](screenshots.md#syncthing-actions-for-dolphin)
* Allows building Syncthing as a library to run it in the same process as the tray/GUI
* English and German localization

## Does this launch or bundle Syncthing itself? What about my existing Syncthing installation?
Syncthing Tray does *not* launch Syncthing itself by default. There should be no interference with your existing
Syncthing installation. You might consider different configurations:

* If you're happy how Syncthing is started on your system so far just tell Syncthing Tray to connect to your currently
  running Syncthing instance in the settings. If you're currently starting Syncthing via systemd you might consider
  enabling the systemd integration in the settings (see section "[Configuring systemd integration](#configuring-systemd-integration)").
* If you would like Syncthing Tray to take care of starting Syncthing for you, you can use the Syncthing launcher
  available in the settings. Note that this is *not* supported when using the Plasmoid.
    * The Linux and Windows builds provided in the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases)
      come with a built-in version of Syncthing which you can consider to use. Keep in mind that automatic updates of Syncthing are
      not possible this way.
    * In any case you can simply point the launcher to the binary of Syncthing (which you have to download/install
      separately).
    * Checkout the "[Configuring the built-in launcher](#configuring-the-built-in-launcher)" section for further details.
* It is also possible to let Syncthing Tray connect to a Syncthing instance running on a different machine.

Note that the experimental UI tailored for mobile devices is more limited. So far it can only start a built-in
version of Syncthing or connect to an externally started Syncthing instance. It will set a custom config/data
directory for Syncthing so any Syncthing instance launched via the mobile UI will not interfere with existing setups.

## Installation and deinstallation
Checkout [the website](https://martchus.github.io/syncthingtray/#downloads-section) for obtaining the executable
or package. This README also lists more options and instructions for building from sources.

If you are using one of the package manager options you should follow the usual workflow of that package manager.

Otherwise, you just have to extract the archive and launch the contained executable. Especially on Windows, please
read the notes on the website before filing any issues. Note that automatic updates haven't been implemented yet.
To uninstall, just delete the executable again.

For further cleanup you may ensure that autostart is disabled (to avoid a dangling autostart entry). You may also
delete the configuration files (see "[Location of the configuration file](#location-of-the-configuration-file)"
section below).

## General remarks on the configuration
You need to configure how Syncthing Tray should connect to Syncthing itself. The previous
section "Does this launch or bundle Syncthing itself…" mentions available options. Additionally,
a wizard is shown on the first launch which can guide though the configuration for common
setups. If you have dismissed the wizard you can still open it at any point via a button on the
top-right corner of the settings dialog.

It may be worthwhile to browse though the pages of the configuration dialog to tweak Syncthing
Tray to your needs, e.g. to turn off notification you may find annoying.

### Location of the configuration file
The configuration file is usually located under `~/.config/syncthingtray.ini` on GNU/Linux and
under `%appdata%\syncthingtray.ini` on Windows. For other platforms and further details,
checkout the
[Qt documentation](https://doc.qt.io/qt-6/qsettings.html#locations-where-application-settings-are-stored)
(Syncthing Tray uses the "IniFormat"). For portable installations it is also possible to place
an empty file called `syncthingtray.ini` directly next to the executable.

You may remove the configuration file under the mentioned location to start from scratch.

Note that this only counts for Syncthing Tray. For Syncthing itself, checkout
[its own documentation](https://docs.syncthing.net/users/config.html).

The Plasmoid is using the same configuration file but in addition also Plasma's configuration
management for settings specific to a concrete instance of the Plasmoid.

The experimental UI tailored for mobile devices is using a distinct configuration which is
located under `~/.config/Martchus/Syncthing Tray` on GNU/Linux and
`/storage/emulated/0/Android/data/io.github.martchus.syncthingtray` on Android and
`%appdata%\Martchus\Syncthing Tray` on Windows. The configuration and database of Syncthing
itself are also located within this directory when Syncthing is launched via the mobile UI.

### Connect to Syncthing via Unix domain socket
When using a Unix domain socket as Syncthing GUI address (e.g. by starting Syncthing with
parameters like `--gui-address=unix://%t/syncthing.socket --skip-port-probing`) you need to
specify the path to the socket as "Local path" in the advanced connection settings. This
setting requires Qt 6.8 or higher. You still need to provide the "Syncthing URL" using the
`unix+http` as scheme (e.g. `unix+http://127.0.0.1:8080` where the host and port are not
actually used). The web view will not work with this, though.

## Single-instance behavior and launch options
This section does *not* apply to the [Android app](#using-the-android-app), the
[Plasmoid](#configuring-plasmoid) and the
[Dolphin integration](#configuring-dolphin-integration).

Syncthing Tray is a single-instance application. So if you try to start a second instance the
second process will only pass arguments to the process that is already running and exit. This
is useful as is prevents one from accidentally launching two Syncthing instances at the same
time via the built-in Syncthing launcher. It also allows showing the triggering certain
actions via certain launch options, see "[Configuring hotkeys](#configuring-hotkeys)" for
details.

Besides that there are a few other notable launch options:

* `--connection [config name] …`:
  Shows tray icons for the specified connection configurations (instead of just a single tray
  icon for the primary connection configuration). Syncthing Tray will still behave as a
  single-instance application so a single process will handle all those tray icons and the
  built-in Syncthing launcher will launch Syncthing only once.
* `--replace`:
  Changes the single-instance behavior so that the already running process is existing and
  the second process continues to run. This is useful to restart Syncthing Tray after
  updating.
* `--new-instance`:
  Disables the single-instance behavior. This can be useful to run two instances of
  Syncthing itself via the built-in launcher in parallel. This only makes sense if those
  two Syncthing instances use a different configuration/database which can be achieved with
  a [portable configuration](#location-of-the-configuration-file).
* `--single-instance`:
  Avoids the creation of a second tray icon if Syncthing Tray is already running. (Without
  this option, Syncthing Tray will still show another tray icon despite its single-instance
  behavior.)
* `--help`:
  Prints all launch options.

Those were just the options of the tray application. Checkout the
"[Using the command-line interface](#using-the-command-line-interface)" section for an
overview of available tooling for the command-line.

## Configuring Plasmoid
The Plasmoid requires installing Syncthing Tray via distribution-specific packaging. It is
*not* available via the generic GNU/Linux download or the Flatpak. Checkout the relevant notes
on the [downloads page](https://martchus.github.io/syncthingtray/#downloads-section) for
available options and details on package names. For further information about supported versions
of Plasma, checkout the "[Supported platforms](#supported-platforms)" section.

The built-in Syncthing launcher is not available in the Plasmoid as it is recommended to rely on
the systemd integration instead.

Once installed, Plasma might need to be restarted for the Plasmoid to be selectable.

The Plasmoid can be added/shown in two different ways:

1. It can be shown as part of the system tray Plasmoid.
    * This is likely the preferred way of showing it and may also happen by default.
    * Whether the Plasmoid is shown as part of the system tray Plasmoid can be configured
      in the settings of the system tray Plasmoid. You can access the settings of the
      system tray Plasmoid from its context-menu which can be opened by right-clicking on
      the arrow for expanding/collapsing.
    * The list of entries in the system tray Plasmoid settings might show an
      invalid/disabled entry for Syncthing in some cases. There should always nevertheless
      also be a valid entry which can be used. See the
      [related issue](https://github.com/Martchus/syncthingtray/issues/239) for details.
    * This way it is also possible to show the icon only in certain states by choosing to
      show it only when important and selecting the states in the Plasmoid's settings.
    * Configuring the size has no effect when the Plasmoid is displayed as part of the
      system tray Plasmoid.
2. It can be added to a panel or the desktop like any other Plasmoid. **Note that under
   recent Plasma versions the configuration no longer seems to be stored persistently.**
   So I recommend using the previous option or following the
   [related issue](https://github.com/Martchus/syncthingtray/issues/339) for workarounds.

This allows you to add multiple instances of the Plasmoid but it is recommended to pick
only one place. For that it makes also most sense to ensure the autostart of the
stand-alone tray application is disabled. Otherwise you would end up having two icons
at the same time (one of the Plasmoid and one of the stand-alone application).

The Plasmoid cannot be closed via its context menu like the stand-alone application.
Instead, you have to disable it in the settings of the system tray Plasmoid as explained
before. If you have added the Plasmoid to a panel or the desktop you can delete it like
any other Plasmoid.

In case the Plasmoid won't show up, checkout the
"[Troubleshooting KDE integration](#troubleshooting-kde-integration)" section below for
further help.

## Configuring Dolphin integration
The Dolphin integration can be enabled/disabled in Dolphin's context menu settings. It will
read Syncthing's API key automatically from its config file. If your Syncthing config file is
not in the default location you need to select it via the corresponding menu action.

## Configuring systemd integration
The next section explains what it is good for and how to use it. If it doesn't work on your
system please read the subsequent sections as well before filing an issue.

### Using the systemd integration
With the system configured correctly and systemd support enabled at build-time the following
features are available:

* Starting and stopping the systemd unit of Syncthing
* Consider the unit status when connecting to the local instance to prevent connection attempts
  when Syncthing isn't running anyways
* Detect when the system has just been resumed from standby to avoid the "Disconnect"
  notification in that case

However, these features are optional. To use them they must be enabled in the settings dialog
first.

It is recommended to enable "Consider unit status …". Note that Syncthing might still not be immediately
ready to serve API requests when the systemd unit turns active. Hence it is still required to configure
a re-connect interval. The re-connect interval will only be in effect while the systemd unit is active.
So despite the re-connect interval there will be no connection attempts while the systemd unit is
inactive. That's all the systemd integration can optimize in that regard.

Be aware that Syncthing Tray assumes by default that the systemd unit is a
[user unit](https://wiki.archlinux.org/index.php/Systemd/User). If you are using
a regular system-wide unit (including those ending with `…@username`) you need to enable the
"System unit" checkbox in the settings. Note that starting and stopping the system-wide Syncthing
unit requires authorization (systemd can ask through PolicyKit).

### Required system configuration
The communication between Syncthing Tray and systemd is implemented using systemd's D-Bus service.
That means systemd's D-Bus service (which is called `org.freedesktop.systemd1`) must be running on
your D-Bus. For [user units](https://wiki.archlinux.org/index.php/Systemd/User) the session D-Bus is
relevant and for regular units (including those ending with `…@username`) the system D-Bus is relevant.

It seems that systemd's D-Bus service is only available when D-Bus itself is started via systemd. That
is by default the case under Arch Linux and openSUSE and likely most other modern distributions where
it is usually started via "socket activation" (e.g. `/usr/lib/systemd/user/dbus.socket` for the session
D-Bus).

All of this counts for the session D-Bus *and* for the system D-Bus although the startup of the session
D-Bus can be screwed up particularly easy. One easy way to screw it up is to start a second instance of
the session D-Bus manually e.g. via `dbus-run-session`. When starting the session D-Bus this way the
systemd integration will *not* work and you will likely end up with two session D-Bus processes. It is
also worth noticing that you do *not* need to set the `DBUS_SESSION_BUS_ADDRESS` variable manually
because the systemd file `dbus.socket` should take care of this.

Note that the Plasma Wayland session screwed things up in the way I've described. This has been fixed with
[Only spawn dbus-run-session if there isn't a session already](https://invent.kde.org/plasma/plasma-workspace/-/merge_requests/128)
but this change might not be available on older distributions.

### Build-time configuration
The systemd integration can be explicitly enabled/disabled at compile time by adding
`-DSYSTEMD_SUPPORT=ON/OFF` to the CMake arguments. If the systemd integration does not work be sure your
version of Syncthing Tray has been compiled with systemd support.

Note for distributors: There will be no hard dependency to systemd in any case. Distributions supporting
alternative init systems do *not* need to provide differently configured versions of Syncthing Tray.
Disabling the systemd integration is mainly intended for systems which do not use systemd at all (e.g.
Windows and MacOS).

## Configuring the built-in launcher
The built-in launcher can be accessed and configured within the settings dialog. It is *not* available
in the Plasmoid. It allows you to launch Syncthing

1. as an *external* process by leaving "Use built-in Syncthing library" *un*checked.
    * When launching Syncthing this way you have to specify the path to an executable, e.g. one you
      have downloaded from the [upstream Syncthing website](https://syncthing.net/downloads). It is also
      possible use the Syncthing version built into Syncthing Tray by pointing it to the Syncthing Tray
      executable and specifying the arguments `syncthing serve`.
    * When launching Syncthing as external process Syncthing Tray does not interfere with
      [Syncthing's configuration for lowering the priority](https://docs.syncthing.net/users/config.html#config-option-options.setlowpriority).
2. as part of the Syncthing Tray UI process by checking "Use built-in Syncthing library".
    * This will always use the Syncthing version built into Syncthing Tray.
    * Launching Syncthing as part of the UI process will interfere with
      [Syncthing's configuration for lowering the priority](https://docs.syncthing.net/users/config.html#config-option-options.setlowpriority).
      You should therefore avoid using this configuration option or start Syncthing as external process
      instead. Otherwise the configuration option might have no effect or will affect the UI of Syncthing
      Tray as well causing it to become slow/unresponsive.
    * This option might not be available on your build of Syncthing Tray, e.g. it is disabled on the
      packages I provide for GNU/Linux distributions as it makes most sense to use the
      distribution-provided version of Syncthing there.

It is recommended to enable "Consider process status …". Note that Syncthing might not be immediately
ready to serve API requests when started. Hence it is still required to configure a re-connect interval.
The re-connect interval will only be in effect while the Syncthing process is running. So despite the
re-connect interval there will be no connection attempts while the Syncthing process is not running.

## Using the command-line interface
Syncthing Tray provides two command-line interfaces:

* The separate executable `syncthingctl` allows to interact with a running instance of Syncthing to
  trigger certain actions like rescans, editing the Syncthing config and more. It complements
  Syncthing's own command-line interface. Invoke `syncthingctl --help` for details.
* The GUI/tray executable `syncthingtray` also exposes a command-line interface to interact with
  a running instance of the GUI/tray. Invoke `syncthingtray --help` for details. Additional remarks:
    * If Syncthing itself is built into Syncthing Tray (like the Linux and Windows builds found in
      the release-section on GitHub) then Syncthing's own command-line interface is exposed via
      `syncthingtray` as well.
    * On Windows, you'll have to use the `syncthingtray-cli` executable to see output in the terminal.
    * The experimental mobile UI can be launched on the desktop with the `qt-quick-gui` sub-command
      when Syncthing Tray was built with support for it.

## Configuring hotkeys
Use the same approach as for launching an arbitrary application via a hotkey in your graphical
environment. Make it invoke

* `syncthingtray --trigger` to show the Qt Widgets based tray menu.
* `syncthingtray --webui` to show the web UI.
* `syncthingctl [...]` to trigger a particular action. See `syncthingctl -h` for details.

The Plasmoid can be shown via a hot-key as well by configuring one in the Plasmoid settings.

## Using the Android app
The Android app requires Android 9 or later. It is mainly tested on Android 14 and 15, though.
Depending on the Android version and vendor-specific limitations you might run into permission
errors and problems with the app being stopped by the OS. For me it works well enough on a
three year old average Samsung device. It probably works on most recent phones except very
low-end devices.

**The Android app is still experimental.** Use it with care and create backups of your
configuration and data before trying it. No builds are provided at this point so you have to
[build it from sources](https://github.com/Martchus/cpp-utilities/blob/master/README.md#remarks-about-building-for-android).
See the section "[Caveats on Android](#caveats-on-android)" below for further limitations.

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
  under "Advanced" → "Syncthing API and web-based GUI".
* The app exposes its private directories via a "document provider" so you can grant other apps
  the permission to access them, e.g. to browse the Syncthing home directory containing the
  Syncthing configuration and database in a file browser that supports "document providers".
  Checkout the "[Using the document provider on Android](#using-the-document-provider-on-android)"
  for details.

### Testing the app without migrating
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

### Migrating data from your existing app setup
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
          have to change the paths of the folders during the import if necassary.
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

### Differences between Syncthing Tray on Android and the Syncthing-Fork app
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
      
### Caveats on Android
While Syncthing Tray basically works on Android, there are still some unresolved issues:

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

### Using the document provider on Android
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

## Download
Checkout the [download section on the website](https://martchus.github.io/syncthingtray/#downloads-section) for an overview.
Keep reading here for a more detailed list.

### Source
See the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases).

### Packages and binaries
* Arch Linux
    * for PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs) or
      [the AUR](https://aur.archlinux.org/packages?SeB=m&K=Martchus)
    * there is also a [binary repository](https://martchus.dyn.f3l.de/repo/arch/ownstuff)
* Tumbleweed, Leap, Fedora
    * RPM \*.spec files and binaries are available via openSUSE Build Service
        * remarks
            * Be sure to add the repository that matches the version of your OS and to keep it
              in sync when upgrading.
            * The linked download pages might be incomplete, use the repositories URL for a full
              list.
            * Old packages might remain as leftovers when upgrading and need to be cleaned up
              manually, e.g. `zypper rm libsyncthingconnector1_1_20 libsyncthingmodel1_1_20 libsyncthingwidgets1_1_20`.
        * latest releases: [download page](https://software.opensuse.org/download.html?project=home:mkittler&package=syncthingtray),
          [repositories URL](https://download.opensuse.org/repositories/home:/mkittler),
          [project page](https://build.opensuse.org/project/show/home:mkittler)
        * Git master: [download page](https://software.opensuse.org/download.html?project=home:mkittler:vcs&package=syncthingtray),
          [repositories URL](https://download.opensuse.org/repositories/home:/mkittler:/vcs),
          [project page](https://build.opensuse.org/project/show/home:mkittler:vcs)
    * available split packages
        * `syncthingtray`/`syncthingtray-qt6`: Qt-widgets based GUI
        * `syncthingplasmoid`/`syncthingplasmoid-qt6`: applet/plasmoid for Plasma desktop
        * `syncthingfileitemaction`/`syncthingfileitemaction-qt6`: Dolphin/KIO integration
        * `syncthingctl`/`syncthingctl-qt6`: command-line interface
* Debian ≥12 "bookworm" and its derivatives (Ubuntu, Pop!_OS, etc, but not Neon)
    * `sudo apt install syncthingtray-kde-plasma` if using KDE Plasma; otherwise, `sudo apt install syncthingtray`.
    * Installation from a Software Centre such as [GNOME Software](https://apps.gnome.org/en-GB/app/org.gnome.Software) or
      [Discover](https://apps.kde.org/en-gb/discover/) should be possible as well.
* Exherbo
    * packages for my other project "Tag Editor" and dependencies could serve as a base and are provided
      by [the platypus repository](https://git.exherbo.org/summer/packages/media-sound/tageditor)
* Gentoo
    * there is a package in [Case_Of's overlay](https://codeberg.org/Case_Of/gentoo-overlay)
* NixOS
    * the package syncthingtray is available from the official repositories
* Void Linux
    * available as split packages from the
      [official repositories](https://voidlinux.org/packages/?q=syncthingtray):
        * `syncthingtray`: GUI and command-line interface
        * `syncthingtray-plasma`: applet/plasmoid for Plasma desktop
        * `syncthingtray-dolphin`: Dolphin/KIO integration
* Other GNU/Linux systems
    * for generic, self-contained binaries checkout the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases)
        * Requires glibc>=2.26, OpenGL and libX11
            * openSUSE Leap 15, Fedora 27, Debian 10 and Ubuntu 18.04 are recent enough (be sure
              the package `libopengl0` is installed on Debian/Ubuntu)
        * Supports X11 and Wayland (set the environment variable `QT_QPA_PLATFORM=xcb` to disable
          native Wayland support if it does not work on your system)
        * Binaries are signed with the GPG key
          [`B9E36A7275FC61B464B67907E06FE8F53CDC6A4C`](https://keyserver.ubuntu.com/pks/lookup?search=B9E36A7275FC61B464B67907E06FE8F53CDC6A4C&fingerprint=on&op=index).
    * a Flatpak is hosted on [Flathub](https://flathub.org/apps/io.github.martchus.syncthingtray)
        * Read the [README of the Flatpak](https://github.com/flathub/io.github.martchus.syncthingtray/blob/master/README.md) for
          caveats and workarounds
        * File any Flatpak-specific issues on [the Flatpak repository](https://github.com/flathub/io.github.martchus.syncthingtray/issues)
* Windows
    * for binaries checkout the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases)
        * Windows SmartScreen will likely block the execution (you'll get a window saying "Windows protected your PC");
          right click on the executable, select properties and tick the checkbox to allow the execution
        * Antivirus software often **wrongly** considers the executable harmful. This is a known problem. Please don't create
          issues about it.
        * The Qt 6 based version is stable and preferable but only supports Windows 10 version 1809 and newer.
        * The Qt 5 based version should still work on older versions down to Windows 7 although this is not regularly checked.
            * On Windows 7 the bundled Go/Syncthing will nevertheless be too new; use a version of Go/Syncthing that is *older*
              than 1.21/1.27.0 instead.
        * The Universal CRT needs to be [installed](https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment#central-deployment).
        * Binaries are signed with the GPG key
          [`B9E36A7275FC61B464B67907E06FE8F53CDC6A4C`](https://keyserver.ubuntu.com/pks/lookup?search=B9E36A7275FC61B464B67907E06FE8F53CDC6A4C&fingerprint=on&op=index).
    * or, using Winget, type `winget install Martchus.syncthingtray` in a Command Prompt window.
    * or, using [Scoop](https://scoop.sh), type `scoop bucket add extras & scoop install extras/syncthingtray`.
    * or, via this [Chocolatey package](https://community.chocolatey.org/packages/syncthingtray), type `choco install syncthingtray`.
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)
* FreeBSD
    * the package syncthingtray is available from [FreeBSD Ports](https://www.freshports.org/deskutils/syncthingtray)
* Mac OS X/macOS
    * the package syncthingtray is available from [MacPorts](https://ports.macports.org/port/syncthingtray/)

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities),
[qtutilities](https://github.com/Martchus/qtutilities) and
[qtforkawesome](https://github.com/Martchus/qtforkawesome) and is built the same way as these libraries.
For basic instructions and platform-specific details checkout the README file of
[c++utilities](https://github.com/Martchus/cpp-utilities).

To avoid building c++utilities/qtutilities/qtforkawesome separately, follow the instructions under
"[Building this straight](#Building-this-straight)". There's also documentation about
[various build variables](https://github.com/Martchus/cpp-utilities/blob/master/doc/buildvariables.md) which
can be passed to CMake to influence the build.

### Further dependencies
The following Qt modules are required (only the latest Qt 5 and Qt 6 version tested): Qt Core, Qt Concurrent,
Qt Network, Qt D-Bus, Qt Gui, Qt Widgets, Qt Svg, Qt WebEngineWidgets/WebKitWidgets

It is recommended to use at least Qt 5.14 to avoid limitations in previous versions (see
"[Known bugs](#known-bugs-and-workarounds)" section).

The built-in web view and therefore the modules WebEngineWidgets/WebKitWidgets are optional (see
section "[Select Qt module for web view and JavaScript](#select-qt-module-for-web-view-and-javascript)").

The Qt Quick UI needs at least Qt 6.8 and also additional Qt modules found in the Qt Declarative repository.
It also contains experimental code for a built-in web view which uses the Qt WebView module. This is optional
and not used at this point.

When building for Android at least Qt 6.9 is required.

To build the plugin for Dolphin integration KIO is also required. To skip building the plugin,
add `-DNO_FILE_ITEM_ACTION_PLUGIN:BOOL=ON` to the CMake arguments.

To build the Plasmoid for the Plasma desktop, the Qt module QML and the KDE Frameworks module Plasma are
required as well. Additionally, the Plasmoid requires the latest Qt version (5.15) for certain Qt Quick features.
To skip building the Plasmoid, add `-DNO_PLASMOID:BOOL=ON` to the CMake arguments.

To specify the major Qt version to use, set `QT_PACKAGE_PREFIX` (e.g. add `-DQT_PACKAGE_PREFIX:STRING=Qt6`
to the CMake arguments). There's also `KF_PACKAGE_PREFIX` for KDE dependencies. Note that KDE integrations
always require the same major Qt version as your KDE installation uses.

---

The following Boost libraries are required: `Boost.Asio`, `Boost.Process`, `Boost.Filesystem`

The launcher uses these libraries by default to handle sub processes correctly (and avoid leftover processes).
Add `-DUSE_BOOST_PROCESS:BOOL:OFF` to the CMake arguments to get rid of the dependency to Boost libraries.
This disables handling sub processes and `QProcess` (from Qt Core) is used instead.

---

To build Syncthing itself as a library Go is required and Syncthing needs to be checked out as a Git submodule.
Checkout the [documentation of Syncthing itself](https://docs.syncthing.net/dev/building#prerequisites) for
details.

Sometimes it can be useful to use a different version of Go then what is provided by the packaging one would
normally use. To download and install a different version of Go into `GOPATH` one can invoke the following
command:

```
go install golang.org/dl/go1.22.11@latest && $GOPATH/src/go/bin/go1.22.11 download`
```

Checkout the [release history of Go](https://go.dev/doc/devel/release) for available versions.

Then the version can be used by adding `-DGO_BIN=$GOPATH/bin/go1.22.11` to the CMake arguments. It is not
necassary to clean an existing build directly. All relevant parts will be re-built as necassary with the
new version.

---

It is also possible to build only the CLI (`syncthingctl`) by adding `-DNO_MODEL:BOOL=ON` and
`-DNO_FILE_ITEM_ACTION_PLUGIN:BOOL=ON` to the CMake arguments. Then only the Qt modules `core`,
`network` and `dbus` are required.

---

To get rid of systemd support, add `-DENABLE_SYSTEMD_SUPPORT_BY_DEFAULT:BOOL=OFF` to the CMake arguments.
In this case the Qt module `dbus` is not required anymore. Note that there is no hard dependency
to systemd in any case.

---

Building the testsuite requires CppUnit and Syncthing itself. Tests will spawn (and eventually terminate)
a test instance of Syncthing that does not affect a possibly existing Syncthing setup on the build host.

### Building this straight
0. Install (preferably the latest version of) the GCC toolchain or Clang, the required Qt modules,
   iconv, CMake and Ninja.
1. Get the sources. For the latest version from Git clone the following repositories:
   ```
   cd "$SOURCES"
   export MSYS=winsymlinks:nativestrict # only required when using MSYS2
   git clone -c core.symlinks=true https://github.com/Martchus/cpp-utilities.git c++utilities
   git clone -c core.symlinks=true https://github.com/Martchus/qtutilities.git
   git clone -c core.symlinks=true https://github.com/Martchus/qtforkawesome.git
   git clone -c core.symlinks=true https://github.com/ForkAwesome/Fork-Awesome.git forkawesome
   git clone -c core.symlinks=true https://github.com/Martchus/syncthingtray.git
   git clone -c core.symlinks=true https://github.com/Martchus/subdirs.git
   ```
   Note that `core.symlinks=true` is only required under Windows to handle symlinks correctly. This requires a
   recent Git version and a filesystem which supports symlinks (NTFS works). Additionally, you need to
   [enable Windows Developer Mode](https://learn.microsoft.com/en-us/gaming/game-bar/guide/developer-mode).
   If you run into "not found" errors on symlink creation use `git reset --hard` within the repository to
   fix this.
2. Configure the build
   ```
   cd "$BUILD_DIR"
   cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="/install/prefix" \
    -DFORK_AWESOME_FONT_FILE="$SOURCES/forkawesome/fonts/forkawesome-webfont.woff2" \
    -DFORK_AWESOME_ICON_DEFINITIONS="$SOURCES/forkawesome/src/icons/icons.yml" \
    "$SOURCES/subdirs/syncthingtray"
   ```
    * Replace `/install/prefix` with the directory where you want to install.
    * Checkout the "[Providing the font file](https://github.com/Martchus/qtforkawesome/#providing-the-font-file)"
      section of qtforkawesome's README for details regarding the
      ForkAwesome-related parameters.
3. Build and install everything in one step:
   ```
   cd "$BUILD_DIR"
   ninja install
   ```
    * If the install directory is not writable, do **not** conduct the build as root. Instead, set `DESTDIR` to a
      writable location (e.g. `DESTDIR="temporary/install/dir" ninja install`) and move the files from there to
      the desired location afterwards.

### Select Qt module for web view and JavaScript
* Add `-DWEBVIEW_PROVIDER:STRING=webkit/webengine/none` to the CMake arguments to use either Qt WebKit (works with
  'revived' version as well), Qt WebEngine or no web view at all. If no web view is used, the Syncthing web UI is
  opened in the default web browser. Otherwise the user can choose between the built-in web view and the web browser.
  Note that this is only about the Qt Widgets based UI. The Qt Quick based UI uses Qt WebView if available.
* Add `-DJS_PROVIDER:STRING=script/qml/none` to the CMake arguments to use either Qt Script, Qt QML or no JavaScript
  engine at all. If no JavaScript engine is used, the CLI does not support scripting configuration changes.

### Troubleshooting KDE integration
All KDE integrations are provided for KDE 5 and 6. The Qt version you have built Syncthing Tray against
must match the KDE version you want to build the integrations for.

If the Dolphin integration or the Plasmoid does not work, check whether the files for those components
have been installed in the right directories.

For instance, under Tumbleweed it looks like this for the Plasmoid:
```
/usr/lib64/qt5/plugins/plasma/applets/libsyncthingplasmoid.so
/usr/share/kservices5/plasma-applet-martchus.syncthingplasmoid.desktop
/usr/share/plasma/plasmoids/martchus.syncthingplasmoid/contents/ui/*.qml
/usr/share/plasma/plasmoids/martchus.syncthingplasmoid/metadata.desktop
/usr/share/plasma/plasmoids/martchus.syncthingplasmoid/metadata.json

```

The files for the Dolphin integration look like this under Tumbleweed:
```
/usr/lib64/qt5/plugins/libsyncthingfileitemaction.so
/usr/share/kservices5/syncthingfileitemaction.desktop
```

These examples were for KDE 5. It looks a bit different for KDE 6. Checkout my Arch Linux and
openSUSE packaging for further examples.

The directory where the `*.so` file needs to be installed to, seems to differ from distribution to
distribution. The right directory for your distribution can be queried using qmake using, e.g.
`qmake-qt5 -query QT_INSTALL_PLUGINS` or `qmake6 -query QT_INSTALL_PLUGINS` depending on the Qt
version. In doubt, just look where other Qt plugins are stored.

The build system is able to do that query automatically. In case this does not work, it is also
possible to specify the directory manually, e.g. for Tumbleweed one would add
`-DQT_PLUGIN_DIR=/usr/lib64/qt6/plugins` to the CMake arguments.

---

Also be sure that the version of the Plasma framework the Plasmoid was built against is *not* newer
than the version actually installed on the system. This can happen if repositories are misconfigured,
e.g. when using Fedora 39 but adding the Fedora 40 repo.

---

If the Plasmoid still won't load, checkout the log of `plasmashell`/`plasmoidviewer`/`plasmawindowed`.
Also consider using strace to find out at which paths the shell is looking for `*.desktop` and
`*.so` files.

For a development setup of the KDE integration, continue reading the subsequent section.

## Contributing, developing, debugging
### Translations
Currently translations for English and German are available. Qt's built-in localization/translation
framework is used under the hood.

Note that `syncthingctl` has not been internationalized yet so it supports only English.

#### Add a new locale
Translations for further locales can be added quite easily:

1. Append a new translation file for the desired locale to the `TS_FILES` list
   in `connector/CMakeLists.txt`, `model/CMakeLists.txt`, `widgets/CMakeLists.txt`,
   `fileitemactionplugin/CMakeLists.txt`, `plasmoid/CMakeLists.txt` and
   `tray/CMakeLists.txt`.
2. Configure a new build, e.g. follow steps under *[Building this straight](#Building-this-straight)*.
3. Conduct a full build or generate only translation files via the `translations` target.
4. New translation files should have been created by the build system under
   `connector/translations`, `model/translations`, `widgets/translations`,
   `fileitemactionplugin/translations`, `plasmoid/translations` and
   `tray/translations` and the `translations` folder of `qtutilities`.
5. Open the files with Qt Linguist to add translations. Qt Linguist is part of
   the [Qt Tools repository](http://code.qt.io/cgit/qt/qttools.git/) and its usage
   is [well documented](http://doc.qt.io/qt-5/linguist-translators.html).

#### Extend/update existing translations
* For English, update the corresponding string literals within the source code.
* If necassary, sync the translation files with the source code like in step `2.`/`3.` of
  "[Add a new locale](#Add-a-new-locale)". Check that no translations have been lost (except ones which are no
  longer required of course).
* Change the strings within the translation files found within the `translations`
  directories like in step `4.`/`5.` of "[Add a new locale](#Add-a-new-locale)".

#### Remarks
* Syncthing Tray displays also text from [qtutilities](https://github.com/Martchus/qtutilities).
  Hence it makes sense adding translations there as well (following the same procedure).
* The CLI `syncthingctl` currently does not support translations.

### Using backend libraries
The contained backend libraries (which provide connecting to Syncthing, data models and more) are written for internal
use within the components contained by this repository.

Hence those libraries do *not* provide a stable ABI/API. If you like to
use them to develop Syncthing integration or tooling with Qt and C++, it makes most sense to contribute it as an additional component
directly to this repository. Then I will be able to take it into account when changing the API.

### KDE integration
Since the Dolphin integration and the Plasmoid are plugins, testing and debugging requires a few extra steps.
See [Testing and debugging Dolphin/KIO plugin with Qt Creator](/fileitemactionplugin/testing.md)
and [Testing and debugging Plasmoid with Qt Creator](/plasmoid/testing.md).

### Logging
It is possible to turn on logging of the underlying library by setting environment variables:

* `LIB_SYNCTHING_CONNECTOR_LOG_ALL`: log everything mentioned in points below
* `LIB_SYNCTHING_CONNECTOR_LOG_API_CALLS`: log calls to Syncthing's REST-API
* `LIB_SYNCTHING_CONNECTOR_LOG_API_REPLIES`: log replies from Syncthing's REST-API (except events)
* `LIB_SYNCTHING_CONNECTOR_LOG_EVENTS`: log events emitted by Syncthing's events REST-API endpoint
* `LIB_SYNCTHING_CONNECTOR_LOG_DIRS_OR_DEVS_RESETTED`: log when folders/devices are internally reset
* `LIB_SYNCTHING_CONNECTOR_LOG_NOTIFICATIONS`: log computed high-level notifications/events
* `SYNCTHINGTRAY_LOG_JS_CONSOLE`: log message from the JavaScript console of the built-in web view

On Windows, you'll have to use the `syncthingtray-cli` executable to see output in the terminal.

### Useful environment variables for development
* `QT_QPA_PLATFORM`: set to `offscreen` to disable graphical output, e.g. to run tests in headless
  environment
* `QT_QPA_PLATFORMTHEME`: the platform theme to use (e.g. `gtk3`) which influences file dialogs and
  other parts of the UI where Qt can make use of native APIs
* `QSG_RHI_BACKEND`: set the underlying graphics API used by the Qt Quick GUI, checkout the
  [Qt documentation](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html#rendering-via-the-qt-rendering-hardware-interface)
  for details
* `QT_QUICK_CONTROLS_STYLE`: the style to use in the Qt Quick GUI, checkout the
  [Qt documentation](https://doc.qt.io/qt-6/qtquickcontrols-styles.html) for available options
* `QT_QUICK_CONTROLS_MATERIAL_THEME`/`QT_QUICK_CONTROLS_UNIVERSAL_THEME`: the theme to use in the Qt
  Quick GUI, the variable and options depend on the style being used
* `LIB_SYNCTHING_CONNECTOR_SYNCTHING_CONFIG_DIR`: override the path where Syncthing Tray's backend expects
  Syncthing's `config.xml` file to be in
* `SYNCTHINGTRAY_FAKE_FIRST_LAUNCH`: assume Syncthing Tray (or the Plasmoid) has been launched for the
  first time
* `SYNCTHINGTRAY_ENABLE_WIP_FEATURES`: enable work-in-progress/experimental features
* `SYNCTHINGTRAY_SINGLE_INSTANCE_ID`: override the identifier used to determine whether another running
  instance of Syncthing Tray is relevant in terms of the single-instance behavior
* `SYNCTHINGTRAY_QML_ENTRY_POINT_PATH`: specifies the Qt Quick GUI entry point to use externally provided
  QML code, e.g. set to something like `G:\projects\main\syncthingtray\tray\gui\qml\AppWindow.qml`; useful
  to hot-reload the Qt Quick GUI with QML code changes with F5 without recompiling and relaunching the
  application
* `SYNCTHINGTRAY_QML_ENTRY_POINT_TYPE`: specifies the Qt Quick GUI entry point to use the specified
  QML module instead of the usual `AppWindow`
* `SYNCTHING_PATH`: override the path of Syncthing's executable when running tests, also recognized by
  the wizard
* `SYNCTHING_PORT`: override the port of the Syncthing test instance spawned when running tests
* `SYNCTHINGTRAY_SYSTEMD_USER_UNIT`: override the name of the systemd user-unit checked by the wizard's
  setup detection
* `SYNCTHINGTRAY_CHROMIUM_BASED_BROWSER`: override the path of the Chromium-based browser to open
  Syncthing in app mode
* `SYNCTHINGTRAY_NATIVE_POPUPS`: set to `0` or `1` to disable/enable native popups in the Qt Quick GUI.
  Note that this is always disabled on platforms where it doesn't work anyway.
* `LIB_SYNCTHING_CONNECTOR_USE_DEPRECATED_ROUTES`: change whether to use deprecated routes (enabled by
  default for compatibility with older Syncthing versions, set to `0` to change the behavior)

## Known bugs and workarounds
The following bugs are caused by dependencies or limitations of certain
platforms. For bugs of Syncthing Tray itself, checkout the issues on GitHub.

### Workaround positioning issues under Wayland
The Qt Widgets based version basically works under Wayland but there are
positioning issues and the settings regarding positioning have no effect (see
"[List of bugs](#List-of-bugs)" section below). One can workaround this limitation by telling the
window manager how to place the window, e.g. under Sway one could add a
configuration like this:

```
for_window [title="^Syncthing Tray( \(.*\))?$"] floating enable, border none, resize set 450 400, move position 916 0
```

Alternatively, one can also configure Syncthing Tray to use a normal window in
the appearance settings. That doesn't fix the positioning issue but then it
looks just like a normal application so not being positioned in the tray area is
less problematic.

You can also select the window type "None". This disables Syncthing Tray's own UI
completely and instead opens Syncthing directly when the tray icon is clicked.

### Tweak GUI settings for dark mode under Windows
The dark mode introduced in Windows 10 does not affect traditional desktop
applications like Syncthing Tray. As of version 6.7 the underlying toolkit Qt
nevertheless provides a style specifically for Windows 11 that supports dark mode.
So as of Qt 6.7 the dark mode should work out of the box on Windows 11. Otherwise
you can select the widgets style "Fusion" under "Qt/Appearance". Then Syncthing
Tray will no longer use native styling of traditional desktop apps and follow the
dark mode setting (as
[Qt 6.5 added dark mode support](https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5)).

It is also recommended to apply some further tweaks:

* Ensure an icon theme that looks good on dark backgrounds is selected. The Windows
  builds provided on GitHub bundle a version of Breeze for light and dark themes. By
  default the version matching the current color palette is selected automatically.
  If you had an icon theme configured explicitly, you may need to manually select a
  different icon theme in the settings under "Qt/Appearance" when enabling dark mode.
* To make Syncthing icons fit better with the dark color palette, configure their
  colors in Syncthing Tray's settings under "Tray/UI icons" and "Tray/System
  icons". The "Use preset" button allows to select pre-defined colors suitable for
  a dark color palette.

When using an older Qt version than 6.5 you will also have to resort to more manual
tweaking:

* To enable dark colors for Syncthing Tray's UI elements, configure a dark color
  palette in Syncthing Tray's settings under "Qt/Appearance". You can download and
  load [dark-palette.ini](https://raw.githubusercontent.com/Martchus/syncthingtray/master/tray/resources/dark-palette.ini)
  as a base and tweak the colors to your liking.
* As of Qt 6.4, dark window borders will be enabled automatically if Windows'
  dark mode setting is enabled and a dark color palette has been selected as
  mentioned in the previous step.
  To enable dark window borders in earlier Qt versions, set the environment
  variable `QT_QPA_PLATFORM` to `windows:darkmode=1` or create a file called
  `qt.conf` next to `syncthingtray.exe` with the contents:
  ```
  [Platforms]
  WindowsArguments = darkmode=1
  ```

When using Syncthing Tray 1.3.x or older, you need to restart Syncthing Tray for
these changes to have any effect. It is not sufficient to close the last window;
the process needs to be restarted.

Note that one can alternatively also enable Windows' "High contrast" setting which
seems to bring back the traditional theming/coloring (which has normally been
[removed](https://superuser.com/questions/949920/window-color-and-appearance-removed-in-win10)).
Unfortunately it doesn't look very nice overall. Checkout
https://github.com/tomasz1986/classic2000 to see how Windows looks like with high
contrast applied, or if you're in need for themes that look at least nicer than
what's shipped with Windows.

### DPI awareness under Windows
Syncthing Tray supports
[PMv2](https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows#per-monitor-and-per-monitor-v2-dpi-awareness)
out of the box as of Qt 6. You may tweak settings according to the
[Qt documentation](https://doc.qt.io/qt-6/highdpi.html#configuring-windows).

### Workaround broken High-DPI scaling of Plasmoid under X11
This problem [has been resolved](https://bugs.kde.org/show_bug.cgi?id=356446#c88) so
make sure you are using an up-to-date Plasma version. Otherwise, setting the environment
variable `PLASMA_USE_QT_SCALING=1` might help.

### List of bugs
* Wayland limitations
    * The tray menu can not be positioned correctly under Wayland because the protocol does not allow setting window positions from
      the client-side (at least I don't know a way to do it). This issue can not be fixed unless Wayland provides an API to set the
      window position to specific coordinates or a system tray icon.
      See discussion on [freedesktop.org](https://lists.freedesktop.org/archives/wayland-devel/2014-August/017584.html).
      Note that the Plasmoid is not affected by this limitation.
    * While the tray menu is shown its entry is shown in the taskbar. Not sure whether there is a way to avoid this.
* Qt limitations and bugs
    * Qt < 6.7:
        * The native style does not look good under Windows 11. Therefore the style "Fusion" is used instead by default.
    * Qt < 6.5:
        * The dark mode introduced in Windows 10 is not supported, see https://bugreports.qt.io/browse/QTBUG-72028.
    * Qt < 5.14
        * Any self-signed certificate is accepted when using Qt WebEngine due to https://bugreports.qt.io/browse/QTBUG-51176.
    * Qt < 5.9:
        * Pausing/resuming folders and devices doesn't work when using scan-intervals with a lot of zeros because of
          Syncthing bug https://github.com/syncthing/syncthing/issues/4001. This has already been fixed on the Qt-side with
          https://codereview.qt-project.org/#/c/187069/. However, the fix is only available in Qt 5.9 and above.
        * Redirections cannot be followed (e.g. from HTTP to HTTPS) because
          `QNetworkRequest::RedirectPolicyAttribute` and `QNetworkRequest::NoLessSafeRedirectPolicy` are not available yet.
    * any Qt version:
        * The tray disconnects from the local instance when the network connection goes down. The network connection must be restored
          or the tray restarted to be able to connect to local Syncthing again. This is caused by Qt bug
          https://bugreports.qt.io/browse/QTBUG-60949.
* KDE limitations
    * High-DPI scaling of Plasmoid is broken under X11 (https://bugs.kde.org/show_bug.cgi?id=356446).
    * Plasma < 5.26.0:
        * The Plasmoid contents are possibly clipped when shown within the system notifications plasmoid.
* Systemd integration
    * This feature relies especially on the system being correctly configured. Checkout the
      "[Required system configuration](#required-system-configuration)" section for details.

## Legal information
### Copyright notice and license
Copyright © 2016-2025 Marius Kittler

All code - unless stated otherwise in a comment on top of the file - is licensed under [GPL-2-or-later](LICENSE). This does *not* apply
to code contained in Git repositories included as Git submodule (which contain their own README and licensing information).

### Attribution for 3rd party content
Syncthing Tray contains icons from various sources:

* Some icons are taken from [Fork Awesome](https://forkaweso.me/Fork-Awesome) (see [their license](https://forkaweso.me/Fork-Awesome/license)).
  These are provided via [qtforkawesome](https://github.com/Martchus/qtforkawesome).
* The Syncthing icons are taken from the [Syncthing](https://github.com/syncthing/syncthing) project.
* The icons on [the website](https://martchus.github.io/syncthingtray) are from
  [Material Design Icons](https://materialdesignicons.com).
* All other icons found in this repository are taken from the [KDE/Breeze](https://invent.kde.org/frameworks/breeze-icons) project.

None of these icons have been (intentionally) modified so no copyright for modifications is asserted.

Some of the code is based on code from other open source projects:

* Code in `tray/gui/quick/quickicon.cpp` and the corresponding header file originates from
  [Kirigami](https://invent.kde.org/frameworks/kirigami). The comments at the beginning of those files state the original
  authors/contributors.
* Parts of `tray/android/src/io/github/martchus/syncthingtray/Util.java` are based on
  [com.nutomic.syncthingandroid.util](https://github.com/Catfriend1/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/util/FileUtils.java).
* The icon files `ic_stat_notify*` under `tray/android/res` and `tray/resources` are taken from
  [syncthing-android](https://github.com/Catfriend1/syncthing-android/tree/main/app/src/main/res).
* The code in `tray/android/src/io/github/martchus/syncthingtray/DocumentsProvider.java` is based on
  [`TermuxDocumentsProvider.java` from Termux](https://github.com/termux/termux-app/blob/master/app/src/main/java/com/termux/filepicker/TermuxDocumentsProvider.java).
* Many of the descriptions used in the Qt Quick GUI are taken from [Syncthing](https://github.com/syncthing/syncthing)
  and [its documentation](https://github.com/syncthing/docs).
* The `uncamel` function used in the Qt Quick GUI is taken from [Syncthing](https://github.com/syncthing/syncthing).

The original code has been modified. Copyright as mentioned in the previous section applies to modifications.
