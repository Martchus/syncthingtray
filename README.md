# [Syncthing](https://github.com/syncthing/syncthing) Tray
* Qt 5-based tray application
* [Dolphin](https://www.kde.org/applications/system/dolphin)/[Plasma](https://www.kde.org/plasma-desktop)
  integration
* command-line interface
* Qt-ish C++ interface to control Syncthing

## Supported platforms
* Designed to work under any desktop environment supported by Qt 5 with tray icon
  support
* No desktop environment specific libraries required (only for optional features)
* Tested under \*
    * Plasma 5 (native "Plasmoid" provided)
    * Openbox/lxqt/LXDE
    * Openbox/qt5ct/Tint2
    * Awesome/qt5ct
    * Cinnamon
    * Windows 10
* Can be shown as regular window if tray icon support is not available

\* If you can confirm it works under other desktop environments, please add it
to the list. Maybe someone could check whether it works under Mac OS X.

## Features
* Provides quick access to most frequently used features but does not intend to replace the official web UI
    * Check state of directories and devices
    * Check current traffic statistics
    * Display further details about directories and devices, like last file, last
      scan, items out of sync, ...
    * Display ongoing downloads
    * Display Syncthing log
    * Trigger re-scan of a specific directory or all directories at once
    * Open a directory with the default file browser
    * Pause/resume a specific device or all devices at once
    * Pause/resume a specific directory
* Shows Syncthing notifications
* Can read the local Syncthing configuration file for quick setup when just connecting to local instance
* Can show the status of the Syncthing systemd unit and allows to start and stop it (see section *Use of systemd*)
* Provides an option to conveniently add the tray to the applications launched when the desktop environment starts
* Can launch Syncthing and syncthing-inotify automatically when started and display stdout/stderr (useful under
  Windows)
* Provides quick access to the official web UI
    * Utilizes either Qt WebKit or Qt WebEngine
    * Can be built without web view support as well (then the web UI is opened in the regular browser)
* Allows quickly switching between multiple Syncthing instances
* Shows notifications via Qt or uses D-Bus notification daemon directly
* Also features a simple command line utility `syncthingctl`
    * Check status
    * Trigger rescan/pause/resume/restart
    * Wait for idle
    * View and modify configuration
    * Supports Bash completion, even for directory and device names
* Also bundles a KIO plugin which shows the status of a Syncthing directory
  and allows to trigger Syncthing actions in Dolphin file manager
    * Rescan selected items
    * Rescan entire Syncthing directory
    * Pause/resume Syncthing directory
    * See also screenshots section
* Also has an implementation as Plasmoid for Plasma 5 desktop
* English and German localization

## Planned features
The tray is still under development; the following features are under construction or planned:

* Build Syncthing as a library to run it in the same process as the tray/GUI
* Create Qt Quick Controls 2 and Kirigami 2 based frontend for mobile devices (focusing on Android)
* Show recently processed items
* Make some notifications configurable on folder level
* Optionally notify for single file updates (https://github.com/Martchus/syncthingtray/issues/7)

## Screenshots

### Under Openbox/Tint2
![Openbox/Tint2](/tray/resources/screenshots/tint2.png?raw=true)

### Under Plasma 5
![Plasma 5 (Plasmoid)](/plasmoid/resources/screenshots/plasmoid-dark.png?raw=true)
![Plasma 5 (Plasmoid and Dolphin integration)](/tray/resources/screenshots/under-construction-3.png?raw=true)
![Plasma 5 (regular GUI)](/tray/resources/screenshots/plasma.png?raw=true)
![Plasma 5 (directory error)](/tray/resources/screenshots/plasma-2.png?raw=true)
![Plasma 5 (dark)](/tray/resources/screenshots/plasma-dark.png?raw=true)

### Settings dialog
![Settings dialog](/tray/resources/screenshots/settings.png?raw=true)

### Web view
![Web view](/tray/resources/screenshots/webview.png?raw=true)
![Web view (dark)](/tray/resources/screenshots/webview-dark.png?raw=true)

### Syncthing actions for Dolphin
![Rescan/pause/status](/fileitemactionplugin/resources/screenshots/dolphin.png?raw=true)

## Hotkeys
To create hotkeys, you can use the same approach as for any other
application. Just make it invoke the `syncthingctl` application with
the arguments for the desired action.

### Hotkey for web UI
Just add `--webui` to the `syncthingtray` arguments to trigger the web UI.
Syncthing Tray ensures that no second instance will be spawned if it is already
running and just trigger the web UI.

## Use of systemd
Use of systemd can be explicitely enabled/disabled by adding
`-DSYSTEMD_SUPPORT=ON/OFF` to the CMake arguments. There will be no hard
dependency to systemd in any case.

With systemd support the tray can start and stop the systemd unit of Syncthing.
It will also take the unit status into account when connecting to the local
instance. So connection attempts can be prevented when Syncthing isn't running
anyways. However, those features are optional. To use them they must be enabled
in the settings dialog first.

Note that this only works when starting Syncthing as user service. This is
described in the [Arch Wiki](https://wiki.archlinux.org/index.php/Systemd/User).

## Download
### Source
See the release section on GitHub.

### Packages and binaries
* Arch Linux
    * for PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs) or
      [the AUR](https://aur.archlinux.org/packages?SeB=m&K=Martchus)
    * for a binary repository checkout [my website](http://martchus.no-ip.biz/website/page.php?name=programming)
* Tumbleweed, Leap, Fedora
    * for RPM \*.spec files and binary repository checkout
      [openSUSE Build Servide](https://build.opensuse.org/project/show/home:mkittler)
    * there's also a [repo with builds of Git master](https://build.opensuse.org/project/show/home:mkittler:vcs)
* Exherbo
    * packages for my other project "Tageditor" and dependencies could serve as a base and are provided
      by [the platypus repository](https://git.exherbo.org/summer/packages/media-sound/tageditor)
* Gentoo
    * packages for my other projects (which are built in the same way) are provided by perfect7gentleman; checkout his
      [repository](https://github.com/perfect7gentleman/pg_overlay)
* Other GNU/Linux systems
    * [AppImage repository for releases on the openSUSE Build Service](https://download.opensuse.org/repositories/home:/mkittler:/appimage/AppImage)
    * [AppImage repository for builds from Git master the openSUSE Build Service](https://download.opensuse.org/repositories/home:/mkittler:/appimage:/vcs/AppImage/)
* Windows
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)
    * for statically linked binaries checkout the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases)
    * [my website](http://martchus.no-ip.biz/website/page.php?name=programming) also contains an occasionally
      updated archive with a dynamically linked executable

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and
[qtutilities](https://github.com/Martchus/qtutilities) and is built the same way as these libaries.
For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).
For building this straight, see the section below. There's also documentation about
[various build variables](https://github.com/Martchus/cpp-utilities/blob/master/doc/buildvariables.md) which
can be passed to CMake to influence the build.

### Further dependencies
The following Qt 5 modules are requried (version 5.6 or newer): core network dbus gui widgets svg webenginewidgets/webkitwidgets

The built-in web view is optional (see section "Select Qt module for WebView").

To build the plugin for Dolphin integration KIO is also requried. Additionally, the Dolphin plugin requires Qt 5.8 or newer. To skip
building the plugin, add `-DNO_FILE_ITEM_ACTION_PLUGIN:BOOL=ON` to the CMake arguments.

To build the Plasmoid for the Plasma 5 desktop, the Qt 5 module QML and the KF5 module
Plasma are required as well. Additionally, the Plasmoid requires Qt 5.8 or newer. To skip
building the Plasmoid, add `-DNO_PLASMOID:BOOL=ON` to the CMake arguments.

It is also possible to build only the CLI (syncthingctl) by adding `-DNO_MODEL:BOOL=ON` and
`-DNO_FILE_ITEM_ACTION_PLUGIN:BOOL=ON` to the CMake arguments. Then only the Qt modules core,
network and dbus are required.

To get rid of systemd support, add `-DENABLE_SYSTEMD_SUPPORT_BY_DEFAULT` to the CMake arguments.
In this case the Qt module D-Bus is not required anymore. Note that there is no hard dependency
to systemd in any case.

Building the testsuite requires CppUnit and Qt 5.8 or higher.

### Building this straight
0. Install (preferably the latest version of) the CGG toolchain or Clang, the required Qt 5 modules and CMake.
1. Get the sources. For the lastest version from Git clone the following repositories:
   ```
   cd $SOURCES
   git clone https://github.com/Martchus/cpp-utilities.git c++utilities
   git clone https://github.com/Martchus/qtutilities.git
   git clone https://github.com/Martchus/syncthingtray.git
   git clone https://github.com/Martchus/subdirs.git
   ```
2. Build and install everything in one step:
   ```
   cd $BUILD_DIR
   cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="/install/prefix" \
    $SOURCES/subdirs/syncthingtray
   make install -j$(nproc)
   ```

### Select Qt module for WebView
* If Qt WebKitWidgets is installed on the system, the tray will link against it. Otherwise it will link against Qt WebEngineWidgets.
* To force usage of Qt WebKit/Qt WebEngine or to disable both add `-DWEBVIEW_PROVIDER=webkit/webengine/none` to the CMake arguments.
* To use Qt WebKit revived/ng, set the web view provider to `webkit`. It works already without any (known) issues.

### BTW: I prefer Qt WebKit (revived/ng version) because
* Currently there is no way to allow a particular self-signed certificate in Qt
  WebEngine. Currently any self-signed certificate is accepted! See:
  https://bugreports.qt.io/browse/QTBUG-51176
* Qt WebEngine can not be built with mingw-w64.
* QWebEngineView seems to eat `keyPressEvent`.
* Qt WebEngine is more buggy in my experience.
* Security issues are not a concern because no other website than the
  Syncthing web UI is shown. Any external links will be opened in the
  regular web browser anyways.

## Contributing
### Adding translations
Currently translations for English and German are available. Further translations
can be added quite easily:

1. Append a new translation file for the desired locale to the `TS_FILES` list
   in `connector/CMakeLists.txt`, `model/CMakeLists.txt`, `widgets/CMakeLists.txt`,
   `plasmoid/CMakeLists.txt` and `tray/CMakeLists.txt`.
2. Trigger a new build, eg. follow steps under *Building this straight*.
3. New translation files should have been created by the build system under
   `connector/translations`, `model/translations` and `tray/translations`.
4. Open the files with Qt Linguist to add translations. Qt Linguist is part of
   the [Qt Tools repository](http://code.qt.io/cgit/qt/qttools.git/) and its usage
   is [well documented](http://doc.qt.io/qt-5/linguist-translators.html).

Note that the CLI `syncthingctl` currently does not support translations.

### Using backend libraries
The contained backend libraries (which provide connecting to Syncthing, data models and more) are written for internal
use whithin the components contained by this repository.

Hence those libraries do *not* provide a stable ABI/API. If you like to
use them to develop Syncthing integration or tooling with Qt and C++, it makes most sense to contribute it as an additional component
directly to this repository. Then I will be able to take it into account when changeing the API.

### KDE integration
Since the Dolphin integration and the Plasmoid are plugins, testing and debugging requires a few extra steps.
See [Testing and debugging Dolphin/KIO plugin with Qt Creator](/fileitemactionplugin/testing.md)
and [Testing and debugging Plasma 5 plasmoid with Qt Creator](/plasmoid/testing.md).

## Known bugs
The following bugs are caused by dependencies and hence tracked externally. For bugs of Syncthing Tray itself, checkout the issues
on GitHub.

* Any self-signed certificate is accepted when using Qt WebEngine due to
  Qt bug https://bugreports.qt.io/browse/QTBUG-51176
* Pausing/resuming folders and devices doesn't work when using scan-intervalls with a lot of zeros
  because of Syncthing bug https://github.com/syncthing/syncthing/issues/4001.
  This has already been fixed on the Qt-side with https://codereview.qt-project.org/#/c/187069/. However, the fix is only
  available in Qt 5.9 and above.
* The tray disconnects from the local instance when the network connection goes down.
  The network connection must be restored or the tray restarted to be able to connect to local
  Syncthing again. This is caused by Qt bug https://bugreports.qt.io/browse/QTBUG-60949.
