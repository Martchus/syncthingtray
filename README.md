# Syncthing Tray
Qt 5-based tray application for [Syncthing](https://github.com/syncthing/syncthing)

## Supported platforms
* Designed to work under any desktop environment supported by Qt 5 with tray icon
support
* No desktop environment specific libraries required
* Tested under
  * Plasma 5
  * Openbox/qt5ct/Tint2
  * Cinnamon
  * Windows 10
* Can be shown as regular window if tray icon support is not available

## Features
* Provides quick access to most frequently used features but does not intend to replace the official web UI
  * Check state of directories and devices
  * Check current traffic statistics
  * Display further details about direcoties and devices, like last file, last
    scan, ...
  * Display ongoing downloads
  * Trigger re-scan of a specific directory or all directories at once
  * Open a directory with the default file browser
  * Pause/resume a specific device or all devices at once
* Shows Syncthing notifications
* Does *not* allow configuring Syncthing itself (currently I do not intend to add this feature as it could cause more harm than good when not implemented correctly)
* Can read the Syncthing configuration file for quick setup when just connecting to local instance
* Provides an option to conveniently add the tray to the applications launched when the desktop environment starts
* Can launch Syncthing when started and display stdout/stderr (useful under Windows)
* Provides quick access to the official web UI
  * Utilizes either Qt WebKit or Qt WebEngine
  * Can be built without web view support as well (then the web UI is opened in the regular browser)
* Allows quickly switching between multiple Syncthing instances
* Features a simple command line utility `syncthingctl` to check Syncthing status and trigger rescan/pause/resume/restart

## Planned features
The tray is still under development; the following features are planned:
* Show recently processed items
* Improve notification handling
* Create Plasmoid for Plasma 5 desktop

## Screenshots

### Under Openbox/Tint2
![Openbox/Tint2](/tray/resources/screenshots/tint2.png?raw=true)

### Under Plasma 5
![Plasma 5](/tray/resources/screenshots/plasma.png?raw=true)
![Plasma 5 (dark)](/tray/resources/screenshots/plasma-dark.png?raw=true)

### Settings dialog
![Settings dialog](/tray/resources/screenshots/settings.png?raw=true)

### Web view
![Web view](/tray/resources/screenshots/webview.png?raw=true)
![Web view (dark)](/tray/resources/screenshots/webview-dark.png?raw=true)

## Hotkey for Web UI
To create a hotkey for the web UI, you can use the same approach as for any other
application. Just add `--webui` to the arguments to trigger the web UI.
Syncthing Tray ensures that no second instance will be spawned if it is already
running.

## Download / binary repository
I currently provide packages for Arch Linux and Windows. Sources for those packages can be found in a
separate [repository](https://github.com/Martchus/PKGBUILDs). For Windows binaries checkout the release section on GitHub or my
[website](http://martchus.no-ip.biz/website/page.php?name=programming).

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and [qtutilities](https://github.com/Martchus/qtutilities) and is built the same way as these libaries. For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities). For building this straight, see the next section.

The following Qt 5 modules are requried: core network gui widgets svg webenginewidgets/webkitwidgets

#### Building this straight
0. Install (preferably the latest version of) g++ or clang, the required Qt 5 modules and CMake.
1. Get the sources. For the lastest version from Git clone the following repositories:

   ```
   cd $SOURCES
   git clone https://github.com/Martchus/cpp-utilities.git
   git clone https://github.com/Martchus/qtutilities.git
   git glone https://github.com/Martchus/syncthingtray.git
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

#### Select Qt module for WebView
* If Qt WebKitWidgets is installed on the system, the tray will link against it. Otherwise it will link against Qt WebEngineWidgets.
* To force usage of Qt WebKit/Qt WebEngine or to disable both add `-DWEBVIEW_PROVIDER=webkit/webengine/none` to the CMake arguments.

#### BTW: I still prefer the deprecated Qt WebKit because
* Currently there is no way to allow a particular self-signed certificate in Qt
  WebEngine. Currently any self-signed certificate is accepted! See:
  https://bugreports.qt.io/browse/QTBUG-51176
* Qt WebEngine can not be built with mingw-w64.
* Qt WebEngine is more buggy in my experience.
* Security issues are not a concern because no other website than the
  Syncthing web UI is shown. Any external links will be opened in the
  regular web browser anyways.
