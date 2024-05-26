# Syncthing Tray
Syncthing Tray provides a tray icon and further platform integrations for
[Syncthing](https://github.com/syncthing/syncthing). Checkout the
[website](https://martchus.github.io/syncthingtray) for an overview.

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

## Supported platforms
Official binaries are provided for Windows and GNU/Linux for the x86_64 architecture and can be download from
the release section on GitHub. This is only a fraction of the available downloads, though. I also provide
further repositories for some GNU/Linux distributions. There are also binaries/repositories provided by other
distributors. For a list with links, checkout the *[Download](#Download)* section of this document.

Syncthing Tray is known to work under:

* Windows 10 and 11
* KDE Plasma
* Openbox using lxqt/LXDE or using Tint2
* GTK-centered desktops such as Cinnamon, GNOME and Xfce (read hints below)
* Awesome
* i3
* macOS
* Deepin Desktop Environment
* Sway/Swaybar/Waybar (with caveats, see "[Known bugs and workarounds](#known-bugs-and-workarounds)")

This does *not* mean Syncthing Tray is actively tested on all those platforms or
desktop environments.

For Plasma 5 and 6, there is in addition to the Qt Widgets based version also a "native"
Plasmoid. Note that the latest version of Syncthing Tray generally also requires the
latest version of Plasma 5 or 6 as no testing on earlier versions is done. Use the Qt
Widgets based version on other Plasma versions. A restart of Plasma might be required for
the Plasmoid to become selectable after installation. Checkout the "[Troubleshooting KDE
integration](#troubleshooting-kde-integration)" section below for further help if it still won't show up.

On GTK-centered desktops have a look at the
[Arch Wiki](https://wiki.archlinux.org/title/Uniform_look_for_Qt_and_GTK_applications)
for how to achieve a more native look and feel. Under GNOME one needs to install
[an extension](https://github.com/ubuntu/gnome-shell-extension-appindicator) for tray icon support.

The section "[Known bugs and workarounds](#known-bugs-and-workarounds)" below contains information and workarounds for
certain caveats.

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
* Allows monitoring the status of the Syncthing systemd unit and to start and stop it (see section "Configuring systemd integration")
* Provides an option to conveniently add the tray to the applications launched when the desktop environment starts
* Can launch Syncthing automatically when started and display stdout/stderr (useful under Windows)
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
    * See also screenshots section
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
    * The Linux and Windows builds provided in the release section on GitHub come with a built-in version of
      Syncthing which you can consider to use. Keep in mind that automatic updates of Syncthing are not possible this
      way.
    * In any case you can simply point the launcher to the binary of Syncthing (which you have to download/install
      separately).
    * Checkout the *[Configuring the built-in launcher](#configuring-the-built-in-launcher)* section for further details.
* It is also possible to let Syncthing Tray connect to a Syncthing instance running on a different machine.

## Installation and deinstallation
Checkout [the website](https://martchus.github.io/syncthingtray/#downloads-section) for obtaining the executable.
This README also lists more options and instructions for building from sources.

If you are using one of the package manager options you should follow the usual workflow of that package manager.

Otherwise, you just have to extract the archive and launch the contained executable. Especially on Windows, please
read the notes on the website before filing any issues. Note that automatic updates haven't been implemented yet.
To uninstall, just delete the executable again.

For further cleanup you may ensure that autostart is disabled (to avoid a dangling autostart entry). You may also
delete the configuration files (see "Location of the configuration file" section below).

## Screenshots
The screenshots are not up-to-date.

### Qt Widgets based GUI under Openbox/Tint2 with dark Breeze theme
![Qt Widgets based GUI under Openbox/Tint2](/tray/resources/screenshots/tint2-dark.png?raw=true)

### Plasmoid (for KDE's Plasma shell)
#### Light theme
![Plasmoid (light theme)](/plasmoid/resources/screenshots/plasmoid-1.png?raw=true)
#### Dark theme
![Plasmoid (dark theme)](/plasmoid/resources/screenshots/plasmoid-2.png?raw=true)

### Icon customization dialog
![Plasmoid (customized icons)](/tray/resources/screenshots/custom-icons.png?raw=true)

### Settings dialog
![Settings dialog](/tray/resources/screenshots/settings.png?raw=true)

### Web view
![Web view](/tray/resources/screenshots/webview.png?raw=true)
![Web view (dark)](/tray/resources/screenshots/webview-dark.png?raw=true)

### Syncthing actions for Dolphin
![Rescan/pause/status](/fileitemactionplugin/resources/screenshots/dolphin.png?raw=true)

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

## Configuring Plasmoid
The Plasmoid can be added/shown in two different ways:

1. It can be shown as part of the system tray Plasmoid.
    * This is likely the preferred way of showing it and may also happen by default.
    * Whether the Plasmoid is shown as part of the system tray Plasmoid can be configured
      in the settings of the system tray Plasmoid. You can access the settings of the
      system tray Plasmoid from its context-menu which can be opened by right-clicking on
      the arrow for expanding/collapsing.
    * This way it is also possible to show the icon only in certain states by choosing to
      show it only when important and selecting the states in the Plasmoid's settings.
    * Configuring the size has no effect when the Plasmoid is displayed as part of the
      system tray Plasmoid.
2. It can be added to a panel or the desktop like any other Plasmoid.

This allows you to add multiple instances of the Plasmoid but it is recommended to pick
only one place. For that it makes also most sense to ensure the autostart of the
stand-alone tray application is disabled. Otherwise you would end up having two icons
at the same time (one of the Plasmoid and one of the stand-alone application).

The Plasmoid cannot be closed via its context menu like the stand-alone application.
Instead, you have to disable it in the settings of the system tray Plasmoid as explained
before. If you have added the Plasmoid to a panel or the desktop you can delete it like
any other Plasmoid.

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
a regular system-wide unit (including those started with `…@username`) you need to enable the
"System unit" checkbox in the settings. Note that starting and stopping the system-wide Syncthing
unit requires authorization (systemd can ask through PolicyKit).

### Required system configuration
The communication between Syncthing Tray and systemd is implemented using systemd's D-Bus service.
That means systemd's D-Bus service (which is called `org.freedesktop.systemd1`) must be running on
your D-Bus. For [user units](https://wiki.archlinux.org/index.php/Systemd/User) the session D-Bus is
relevant and for regular units (including those started with `…@username`) the system D-Bus is relevant.

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
The built-in launcher can be accessed and configured within the settings dialog. The GUI should be
self-explaining.

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

## Configuring hotkeys
Use the same approach as for launching an arbitrary application via a hotkey in your graphical
environment. Make it invoke

* `syncthingtray --trigger` to show the Qt Widgets based tray menu.
* `syncthingtray --webui` to show the web UI.
* `syncthingctl [...]` to trigger a particular action. See `syncthingctl -h` for details.

The Plasmoid can be shown via a hot-key as well by configuring one in the Plasmoid settings.

## Download
### Source
See the release section on GitHub.

### Packages and binaries
* Arch Linux
    * for PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs) or
      [the AUR](https://aur.archlinux.org/packages?SeB=m&K=Martchus)
    * there is also a [binary repository](https://martchus.no-ip.biz/repo/arch/ownstuff)
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
* Debian 12 "bookworm" and its derivatives (Ubuntu, Pop!_OS, Neon, etc.)
    * `sudo apt install syncthingtray-kde-plasma` if using KDE Plasma; otherwise, `sudo apt install syncthingtray`.
    * Installation from a Software Centre such as [GNOME Software](https://apps.gnome.org/en-GB/app/org.gnome.Software) or
      [Discover](https://apps.kde.org/en-gb/discover/) may be possible as well.
    * [backport](https://backports.debian.org/) to Debian 11 "bullseye" available on request.
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
        * the Qt 6 based version is stable and preferable but only supports Windows 10 version 1809 and newer
        * the Qt 5 based version should still work on older versions down to Windows 7 although this is not regularly checked
            * on Windows 7 the bundled Go/Syncthing will nevertheless be too new; use a version of Go/Syncthing that is *older*
              than 1.21/1.27.0 instead
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
For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).

To avoid building c++utilities/qtutilities/qtforkawesome separately, follow the instructions under
"[Building this straight](#Building-this-straight)". There's also documentation about
[various build variables](https://github.com/Martchus/cpp-utilities/blob/master/doc/buildvariables.md) which
can be passed to CMake to influence the build.

### Further dependencies
The following Qt modules are required (only the latest Qt 5 and Qt 6 version tested): `core`, `concurrent`,
`network`, `dbus`, `gui`, `widgets`, `svg`, `webenginewidgets`/`webkitwidgets`

It is recommended to use at least Qt 5.14 to avoid limitations in previous versions (see *Known bugs* section).

The built-in web view and therefore the modules webenginewidgets/webkitwidgets are optional (see
section "Select Qt module for web view and JavaScript").

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
    * Checkout the [Providing the font file](https://github.com/Martchus/qtforkawesome/#providing-the-font-file)
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
* Add `-DJS_PROVIDER:STRING=script/qml/none` to the CMake arguments to use either Qt Script, Qt QML or no JavaScript
  engine at all. If no JavaScript engine is used, the CLI does not support scripting configuration changes.

#### Limitations of Qt WebEngine compared to Qt WebKit
* When using a version of Qt older than 5.14 there is no way to allow only a particular self-signed certificate in Qt
  WebEngine. That means any self-signed certificate is accepted! See: https://bugreports.qt.io/browse/QTBUG-51176
* Qt WebEngine can not be built with GCC/mingw-w64 for Windows.
* Security issues are not a concern because no other website than the Syncthing web UI is shown. Any external links
  will be opened in the regular web browser anyways.

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
distribution. The right directory for your distribution can be queried from qmake using
`qmake-qt5 -query QT_INSTALL_PLUGINS`. In doubt, just look where other Qt plugins are stored.

Actually, the build system should be able to do that query automatically. It is also possible to
specify the directory manually, e.g. for Tumbleweed one would add
`-DQT_PLUGIN_DIR=/usr/lib64/qt5/plugins` to the CMake arguments.

---

Also be sure that the version of the plasma framework the plasmoid was built against is *not* newer
than the version actually installed on the system. That can for instance easily happen when using
`tumbleweed-cli` for sticking to a previous snapshot but having the latest version of the plasmoid
from my home repository installed.

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

### Useful environment variables for development
* `QT_QPA_PLATFORM`: set to `offscreen` to disable graphical output, e.g. to run tests in headless
  environment
* `LIB_SYNCTHING_CONNECTOR_SYNCTHING_CONFIG_DIR`: override the path where Syncthing Tray's backend expects
  Syncthing's `config.xml` file to be in
* `SYNCTHINGTRAY_FAKE_FIRST_LAUNCH`: assume Syncthing Tray (or the Plasmoid) has been launched for the
  first time
* `SYNCTHINGTRAY_ENABLE_WIP_FEATURES`: enable work-in-progress/experimental features
* `SYNCTHING_PATH`: override the path of Syncthing's executable when running tests
* `SYNCTHING_PORT`: override the port of the Syncthing test instance spawned when running tests
* `SYNCTHINGTRAY_SYSTEMD_USER_UNIT`: override the name of the systemd user-unit checked by the wizard's
  setup detection
* `SYNCTHINGTRAY_CHROMIUM_BASED_BROWSER`: override the path of the Chromium-based browser to open
  Syncthing in app mode
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
    * This feature relies especially on the system being correctly configured. Checkout the *Required system configuration* section
      for details.

## Copyright notice and license
Copyright © 2016-2024 Marius Kittler

All code is licensed under [GPL-2-or-later](LICENSE). This does *not* apply to
code contained in Git repositories included as Git submodule (which contain
their own README and licensing information).

## Attribution for 3rd party content
* Some icons are taken from [Fork Awesome](https://forkaweso.me/Fork-Awesome) (see [their license](https://forkaweso.me/Fork-Awesome/license)). These are provided via [qtforkawesome](https://github.com/Martchus/qtforkawesome).
* The Syncthing icons are taken from the [Syncthing](https://github.com/syncthing/syncthing) project.
* All other icons found in this repository are taken from the [KDE/Breeze](https://invent.kde.org/frameworks/breeze-icons) project.

None of these icons have been (intentionally) modified so no copyright for modifications is asserted.
