# [Syncthing](https://github.com/syncthing/syncthing) Tray
* Qt 5-based tray application
* [Dolphin](https://www.kde.org/applications/system/dolphin)/[Plasma](https://www.kde.org/plasma-desktop)
  integration
* command-line interface
* Qt-ish C++ interface to control Syncthing

---

Checkout the [official forum thread](https://forum.syncthing.net/t/yet-another-syncthing-tray) for discussions
and announcement of new features.

Issues can be created on GitHub but please read the "Known bugs" and "Planned features" sections in this document
before.

I provide binaries/repositories for some platforms. There are also binaries/repositories provided by other
distributors. For a list with links, checkout the *Download* section of this document. The release section on
GitHub only contains a fraction of the available options.

## Supported platforms
* Designed to work under any desktop environment supported by Qt 5 with tray icon support
* No desktop environment specific libraries required (only for optional features/integrations)
* Tested under
    * X Window System
        * Plasma 5 (beside Qt Widgets based version there is a native "Plasmoid")
        * Openbox/lxqt/LXDE
        * Openbox/qt5ct/Tint2
        * Awesome/qt5ct
        * Cinnamon (native look and feel using [adwaita-qt](https://github.com/FedoraQt/adwaita-qt))
        * Deepin Desktop Environment
        * Xfce
    * Wayland
        * Plasma 5
            * native "Plasmoid" works well
            * for Qt Widgets based version see note below
        * other desktop environments
            * for Qt Widgets based version see note below
            * besides, the Qt Widgets based version would only work if the platform integration plugin
              used for Qt 5 applications uses the D-Bus protocol (or whatever is supported by your system
              tray) to show the tray icon
    * Windows 10
    * macOS 10.14 Mojave
* Can be shown as regular window if tray icon support is not available

The Qt Widgets based version basically works under Wayland but there are positioning issues (see known bugs
section).

If you can confirm it works under other desktop environments, please add it to the list.

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
    * View recent history of changes (done locally and remotely)
* Shows notifications
    * The notification to show is configurable
    * Uses Qt's notification support or a D-Bus notification daemon directly
* Reads connection parameters from Syncthing config file for quick setup (when just connecting to local instance)
* Allows monitoring the status of the Syncthing systemd unit to start and stop it (see section *Use of systemd*)
* Provides an option to conveniently add the tray to the applications launched when the desktop environment starts
* Can launch Syncthing automatically when started and display stdout/stderr (useful under Windows)
* Provides quick access to the official web UI
    * Utilizes either Qt WebEngine or Qt WebKit
    * Can be built without web view support as well (then the web UI is opened in the regular browser)
* Allows switching quickly between multiple Syncthing instances
* Also features a simple command line utility `syncthingctl`
    * Check status
    * Trigger rescan/pause/resume/restart
    * Wait for idle
    * View and modify raw configuration
    * Supports Bash completion, even for directory and device names
* Also bundles a KIO plugin which shows the status of a Syncthing directory and allows to trigger Syncthing actions
  in Dolphin file manager
    * Rescan selected items
    * Rescan entire Syncthing directory
    * Pause/resume Syncthing directory
    * See also screenshots section
* Also has an implementation as Plasmoid for Plasma 5 desktop
* Allows building Syncthing as a library to run it in the same process as the tray/GUI (optional build configuration
  which is not enabled by default)
* English and German localization

## Does this launch or bundle Syncthing itself? What about my existing Syncthing installation?
Syncthing Tray does *not* launch Syncthing itself by default. There should be no interface with your existing
Syncthing installation. You might consider different configurations:

* If you're happy how Syncthing is started on your system so far just tell Syncthing Tray to connect to your currently
  running Syncthing instance in the settings. If you're currently starting Syncthing via systemd you might consider
  enabling the systemd integration in the settings (see section *Use of systemd*).
* If you would like Syncthing Tray to take care of starting Syncthing for you, you can use the Syncthing launcher
  available in the settings.
    * The Windows builds provided in the release section on GitHub come with a built-in version of Syncthing
      which you can consider to use. Keep in mind that automatic updates of Syncthing are not possible this way.
    * In any case you can simply point the launcher to the binary of Syncthing (which you have to install separately).
* It is also possible to let Syncthing Tray connect to a Syncthing instance running on a different machine.

## Screenshots
The screenshots are not up-to-date.

### Qt Widgets based GUI under Openbox/Tint2 with dark Breeze theme
![Qt Widgets based GUI under Openbox/Tint2](/tray/resources/screenshots/tint2-dark.png?raw=true)

### Under Plasma 5
#### Light theme
![Plasmoid (light theme)](/plasmoid/resources/screenshots/plasmoid-1.png?raw=true)
#### Dark theme
![Plasmoid (dark theme)](/plasmoid/resources/screenshots/plasmoid-2.png?raw=true)
#### Customized icons
![Plasmoid (customized icons)](/plasmoid/resources/screenshots/plasmoid-3-custom-icons.png?raw=true)

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
It can also take the unit status into account when connecting to the local
instance. So connection attempts can be prevented when Syncthing isn't running
anyways. However, those features are optional. To use them they must be enabled
in the settings dialog first.

Syncthing Tray assumes by default that the systemd unit is a
[user unit](https://wiki.archlinux.org/index.php/Systemd/User). If you are using
a system-wide unit you need to enable the "System unit" checkbox in the settings.
Note that starting and stopping the system-wide Syncthing unit requires
authorization (systemd can ask though PolicyKit).

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
* Other GNU/Linux systems
    * [AppImage repository for releases on the openSUSE Build Service](https://download.opensuse.org/repositories/home:/mkittler:/appimage/AppImage)
    * [AppImage repository for builds from Git master the openSUSE Build Service](https://download.opensuse.org/repositories/home:/mkittler:/appimage:/vcs/AppImage/)
* Windows
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)
    * for statically linked binaries checkout the [release section on GitHub](https://github.com/Martchus/syncthingtray/releases)
* NixOS
    * the package syncthingtray is available from the official repositories
* FreeBSD
    * the package syncthingtray is available from [FreeBSD Ports](https://www.freshports.org/deskutils/syncthingtray)
* Mac OS X/macOS
    * the package syncthingtray is available from [MacPorts](https://ports.macports.org/port/syncthingtray/)
* Exherbo
    * packages for my other project "Tag Editor" and dependencies could serve as a base and are provided
      by [the platypus repository](https://git.exherbo.org/summer/packages/media-sound/tageditor)
* Gentoo
    * packages for my other project "Tag Editor" and dependencies could serve as a base and are provided
      by [perfect7gentleman's repository](https://github.com/perfect7gentleman/pg_overlay)

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and
[qtutilities](https://github.com/Martchus/qtutilities) and is built the same way as these libaries.
For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).
For building this straight, see the section below. There's also documentation about
[various build variables](https://github.com/Martchus/cpp-utilities/blob/master/doc/buildvariables.md) which
can be passed to CMake to influence the build.

### Further dependencies
The following Qt 5 modules are requried (version 5.6 or newer): core network dbus gui widgets svg webenginewidgets/webkitwidgets

It is recommended to use at least Qt 5.14 to avoid limitations in previous versions (see *Known bugs* section).

The built-in web view and therefore the modules webenginewidgets/webkitwidgets are optional (see section *Select Qt module for WebView*).

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
* QWebEngineView seems to eat `keyPressEvent`.
* Security issues are not a concern because no other website than the Syncthing web UI is shown. Any external links
  will be opened in the regular web browser anyways.

### Troubleshooting KDE integration
If the Dolphin integration or the Plasmoid don't work, check whether the files for those components
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

The directory the `*.so` file needs to be installed to seems to differ from distribution to
distribution. The right directory for your distribution can be queried from qmake using
`qmake-qt5 -query QT_INSTALL_PLUGINS`. In doubt, just look where other Qt 5 plugins are stored.

Actually the build system should be able to do that query automatically. It is also possible to
specify the directory manually, e.g. for Tumbleweed one would add
`-DQT_PLUGIN_DIR=/usr/lib64/qt5/plugins` to the CMake arguments.

---

Also be sure that the version of the plasma framework the plasmoid was built against is *not* newer
than the version actually installed on the system. That can for instance easily happen when using
`tumbleweed-cli` for sticking to a previous snapshot but having the lastest version of the plasmoid
from my home repository installed.

---

If the Plasmoid still won't load, checkout the log of `plasmashell`/`plasmoidviewer`/`plasmawindowed`.
Also consider using strace to find out at which paths the shell is looking for `*.desktop` and
`*.so` files.

For a development setup of the KDE integration, continue reading the subsequent section.

## Contributing
### Adding translations
Currently translations for English and German are available. Further translations
can be added quite easily:

1. Append a new translation file for the desired locale to the `TS_FILES` list
   in `connector/CMakeLists.txt`, `model/CMakeLists.txt`, `widgets/CMakeLists.txt`,
   `fileitemactionplugin/CMakeLists.txt`, `plasmoid/CMakeLists.txt` and
   `tray/CMakeLists.txt`.
2. Trigger a new build, eg. follow steps under *Building this straight*.
3. New translation files should have been created by the build system under
   `connector/translations`, `model/translations`, `widgets/translations`,
   `fileitemactionplugin/translations`, `plasmoid/translations` and
   `tray/translations`.
4. Open the files with Qt Linguist to add translations. Qt Linguist is part of
   the [Qt Tools repository](http://code.qt.io/cgit/qt/qttools.git/) and its usage
   is [well documented](http://doc.qt.io/qt-5/linguist-translators.html).

#### Remarks
* Syncthing Tray displays also text from [qtutilities](https://github.com/Martchus/qtutilities).
  Hence it makes sense adding translations there as well (following the same procedure).
* The CLI `syncthingctl` currently does not support translations.

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

## Planned features
The tray is still under development; the following features are under construction or planned:

* Create Qt Quick Controls 2 and Kirigami 2 based frontend for mobile devices (focusing on Android)
* Make some notifications configurable on folder level
* Optionally notify for single file updates (https://github.com/Martchus/syncthingtray/issues/7)

## Known bugs
The following bugs are caused by dependencies and hence tracked externally. For bugs of Syncthing Tray itself, checkout the issues
on GitHub.

* Wayland limitations
    * The tray menu can not be positioned correctly under Wayland because the protocol does not allow setting window positions from
      the client-side (at least I don't know a way to do it). This issue can not be fixed unless Wayland provides an API to set the
      window position to specific coordinates or a system tray icon.
      See discussion on [freedesktop.org](https://lists.freedesktop.org/archives/wayland-devel/2014-August/017584.html).
      Note that the Plasmoid is not affected by this limitation.
    * While the tray menu is shown its entry is shown in the taskbar. Not sure whether there is a way to avoid this.
* Qt bugs
    * Qt < 5.14
        * Any self-signed certificate is accepted when using Qt WebEngine due to https://bugreports.qt.io/browse/QTBUG-51176.
    * Qt < 5.9:
        * Pausing/resuming folders and devices doesn't work when using scan-intervalls with a lot of zeros because of
          Syncthing bug https://github.com/syncthing/syncthing/issues/4001. This has already been fixed on the Qt-side with
          https://codereview.qt-project.org/#/c/187069/. However, the fix is only available in Qt 5.9 and above.
    * any Qt version:
        * The tray disconnects from the local instance when the network connection goes down. The network connection must be restored
          or the tray restarted to be able to connect to local Syncthing again. This is caused by Qt bug
          https://bugreports.qt.io/browse/QTBUG-60949.

## Attribution for 3rd party content
* Some icons are taken from the Syncthing project.
* Some icons are taken from [Font Awesome](https://fontawesome.com) (see [their license](https://fontawesome.com/license)).
* Fallback icons are taken from KDE/Breeze project.
