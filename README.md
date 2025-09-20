# Syncthing Tray
Syncthing Tray provides a tray icon and further platform integrations for
[Syncthing](https://github.com/syncthing/syncthing). Checkout the
[website](https://martchus.github.io/syncthingtray) for an overview and
screenshots.

The following integrations are provided:

* Tray application (using the Qt framework)
* Context menu extension for the [Dolphin](https://www.kde.org/applications/system/dolphin) file manager
* Plasmoid for [KDE Plasma](https://www.kde.org/plasma-desktop)
* [Command-line interface](docs/cli.md)
* Qt-ish C++ library

---

Checkout the [official forum thread](https://forum.syncthing.net/t/yet-another-syncthing-tray) for discussions
and announcement of new features.

This README document currently serves as the only and main documentation. So read on for details about
the configuration. If you are not familiar with Syncthing itself already you should also have a look at
the [Syncthing documentation](https://docs.syncthing.net) as this README is only going to cover the
Syncthing Tray integration.

Issues can be created on GitHub but please check the
[documentation on known bugs and workarounds](docs/known_bugs_and_workarounds.md) before.

Syncthing Tray works with Syncthing v1 and v2 (and probably v0). Syncthing Tray is maintained, and updates will
be made to support future Syncthing versions as needed.

## Supported platforms
Official binaries are provided for Windows (for i686, x86_64 and aarch64) and GNU/Linux (for x86_64) and can be
download from the [website](https://martchus.github.io/syncthingtray/#downloads-section) and the
[release section on GitHub](https://github.com/Martchus/syncthingtray/releases). This is only a fraction of
the available downloads, though. I also provide further repositories for some GNU/Linux distributions. There are
also binaries/repositories provided by other distributors. For a list with links, checkout the
"[Download](#download)" section of this document.

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
* Android (still experimental)

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
extension the Syncthing Tray UI shown in the [screenshots](docs/screenshots.md) is only shown by *double*-clicking
the icon. If your system tray is unable to show the Syncthing Tray UI at all like on COSMIC you can still use
Syncthing Tray for the tray icon and basic functionality accessible via the menu.

Note that under Wayland-based desktops there will be positioning issues. The Plasmoid is not affected
by this, though.

The [documentation on known bugs and workarounds](docs/known_bugs_and_workarounds.md)
contains further information and workarounds for certain platform-specific issues like the positioning issues under
Wayland.

Documentation on how to use Syncthing Tray on Android can be found in a [separate document](docs/android.md).

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
    * See also the [screenshots](docs/screenshots.md#syncthing-actions-for-dolphin)
* Allows building Syncthing as a library to run it in the same process as the tray/GUI
* English and German localization

## Does this launch or bundle Syncthing itself? What about my existing Syncthing installation?
Syncthing Tray does *not* launch Syncthing itself by default. There should be no interference with your existing
Syncthing installation. You might consider different configurations:

* If you're happy how Syncthing is started on your system so far just tell Syncthing Tray to connect to your currently
  running Syncthing instance in the settings.
    * When starting Syncthing via systemd it is recommended to enable the systemd integration in the settings (see section
      "[Configuring systemd integration](#configuring-systemd-integration)").
    * When starting Syncthing by other means (e.g. as Windows service) there are no further integrations provided. Hence,
      Syncthing Tray cannot know whether Syncthing is expected to be running or not. It will therefore unconditionally
      attempt to connect with Syncthing continuously as-per the configurable re-connect interval. It will also
      unconditionally notify when disconnecting from Syncthing if this kind of notification is enabled (so it makes perhaps
      most sense to disable it).
* If you would like Syncthing Tray to take care of starting Syncthing for you, you can use the Syncthing launcher
  available in the settings. Note that this is *not* supported when using the Plasmoid.
    * The Linux and Windows builds provided in the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases)
      come with a built-in version of Syncthing which you can consider to use. Note that the built-in version of Syncthing
      will be only updated when you update Syncthing Tray (either manually or via the its updater). The update feature of
      Syncthing itself is not available this way.
    * In any case you can simply point the launcher to the binary of Syncthing which you have to download/install separately.
      This way Syncthing can be (but also has to be) updated independently of Syncthing Tray, e.g. using Syncthing's own
      update feature.
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
read the notes on the website before filing any issues. To uninstall, just delete the executable again.

Notifications about updates can be enabled in the settings which also allow upgrading to a new version if available.
This simply replaces the executable at the location where you put it so this location needs to be writable. The old
executable is renamed/preserved as a backup and you can simply rename it back if you need to go back to the previous
version.

For further cleanup you may ensure that autostart is disabled (to avoid a dangling autostart entry). You may also
delete the configuration files (see "[Location of the configuration file](#location-of-the-configuration-file)"
section below).

## Configuration
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

### Single-instance behavior and launch options
This section does *not* apply to the [Android app](#using-the-android-app), the
[Plasmoid](#configuring-plasmoid) and the
[Dolphin integration](#configuring-dolphin-integration).

Syncthing Tray is a single-instance application. So if you try to start a second instance the
second process will only pass arguments to the process that is already running and exit. This
is useful as is prevents one from accidentally launching two Syncthing instances at the same
time via the built-in Syncthing launcher. It also allows triggering certain actions via launch
options, see "[Configuring hotkeys](#configuring-hotkeys)" for details.

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

Those were just the options of the tray application. Checkout
"[Using the command-line interface](docs/cli.md)" for an
overview of available tooling for the command-line.

### Configuring Plasmoid
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

### Configuring Dolphin integration
The Dolphin integration can be enabled/disabled in Dolphin's context menu settings. It will
read Syncthing's API key automatically from its config file. If your Syncthing config file is
not in the default location you need to select it via the corresponding menu action.

### Configuring systemd integration
The next section explains what it is good for and how to use it. If it doesn't work on your
system please read the subsequent sections as well before filing an issue.

#### Using the systemd integration
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

#### Required system configuration
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

### Configuring the built-in launcher
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

### Configuring hotkeys
Use the same approach as for launching an arbitrary application via a hotkey in your graphical
environment. Make it invoke

* `syncthingtray --trigger` to show the Qt Widgets based tray menu.
* `syncthingtray --webui` to show the web UI.
* `syncthingctl [...]` to trigger a particular action. See `syncthingctl -h` for details.

The Plasmoid can be shown via a hot-key as well by configuring one in the Plasmoid settings.

## Using the command-line interface
Checkout `syncthingctl --help` and `syncthingtray --help` for available options. More details
can be found in the [CLI documentation](docs/cli.md).

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
              the packages `libglx0`, `libopengl0` and `libegl1` are installed on Debian/Ubuntu)
        * Supports X11 and Wayland (set the environment variable `QT_QPA_PLATFORM=xcb` to disable
          native Wayland support if it does not work on your system)
        * The built-in web view is not available in these builts as it would require shipping a full web browser engine.
          Syncthing Tray can still show the official web-based UI in a dedicated window via a Chromium-based browser you have
          already installed using its "app mode". Alternatively, use the distribution-specific builds provided for Arch Linux,
          openSUSE and Fedora which come with the built-in web view enabled.
        * The archive is signed with the GPG key
          [`B9E36A7275FC61B464B67907E06FE8F53CDC6A4C`](https://keyserver.ubuntu.com/pks/lookup?search=B9E36A7275FC61B464B67907E06FE8F53CDC6A4C&fingerprint=on&op=index) for manual verification.
        * The executable is signed in addition using ECDSA for verification by the updater. The public key can be found
          [in the source code](https://github.com/Martchus/syncthingtray/blob/master/tray/application/main.cpp) and verification
          is possible with `stsigtool` or OpenSSL.
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
        * The default version is using Qt 6 and hence preferred on modern versions of Windows. The oldest version of Windows it
          supports is 64-bit Windows 10 version 1809.
        * The Qt 5 based version should still work on older versions down to Windows 7 although this is not regularly checked.
            * On Windows 7 the bundled Go/Syncthing will nevertheless be too new; use a version of Go/Syncthing that is *older*
              than 1.21/1.27.0 instead.
        * The Universal CRT needs to be [installed](https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment#central-deployment).
        * The built-in web view is not available in these builts as it would require shipping a full web browser engine.
          Syncthing Tray can still show the official web-based UI in a dedicated window via a Chromium-based browser you have
          already installed (e.g. Edge or Chrome) using its "app mode".
        * The archive is signed with the GPG key
          [`B9E36A7275FC61B464B67907E06FE8F53CDC6A4C`](https://keyserver.ubuntu.com/pks/lookup?search=B9E36A7275FC61B464B67907E06FE8F53CDC6A4C&fingerprint=on&op=index) for manual verification.
        * The executable is signed in addition using ECDSA for verification by the updater. The public key can be found
          [in the source code](https://github.com/Martchus/syncthingtray/blob/master/tray/application/main.cpp) and verification
          is possible with `stsigtool` or OpenSSL.
    * or, using Winget, type `winget install Martchus.syncthingtray` in a Command Prompt window.
    * or, using [Scoop](https://scoop.sh), type `scoop bucket add extras & scoop install extras/syncthingtray`.
    * or, via this [Chocolatey package](https://community.chocolatey.org/packages/syncthingtray), type `choco install syncthingtray`.
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)
* FreeBSD
    * the package syncthingtray is available from [FreeBSD Ports](https://www.freshports.org/deskutils/syncthingtray)
* Mac OS X/macOS
    * the package syncthingtray is available from [MacPorts](https://ports.macports.org/port/syncthingtray/)

## Contributing, building, developing, building, debugging
There is separate [documentation on these topics](docs/devel.md).

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

* Code in `syncthingwidgets/quick/quickicon.cpp` and the corresponding header file originates from
  [Kirigami](https://invent.kde.org/frameworks/kirigami). The comments at the beginning of those files state the original
  authors/contributors.
* Parts of `tray/android/src/io/github/martchus/syncthingtray/Util.java` are based on
  [com.nutomic.syncthingandroid.util](https://github.com/Catfriend1/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/util/FileUtils.java) and
  [com.nutomic.syncthingandroid.service](https://github.com/Catfriend1/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/service/SyncthingRunnable.java).
* The icon files `ic_stat_notify*` under `tray/android/res` and `tray/resources` are taken from
  [syncthing-android](https://github.com/Catfriend1/syncthing-android/tree/main/app/src/main/res).
* The code in `tray/android/src/io/github/martchus/syncthingtray/DocumentsProvider.java` is based on
  [`TermuxDocumentsProvider.java` from Termux](https://github.com/termux/termux-app/blob/master/app/src/main/java/com/termux/filepicker/TermuxDocumentsProvider.java).
* Many of the descriptions used in the Qt Quick GUI are taken from [Syncthing](https://github.com/syncthing/syncthing)
  and [its documentation](https://github.com/syncthing/docs).
* The `uncamel` function used in the Qt Quick GUI is taken from [Syncthing](https://github.com/syncthing/syncthing).

The original code has been modified. Copyright as mentioned in the previous section applies to modifications.
