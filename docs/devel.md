# Contributing, building, developing, debugging
The follow sections explain how to build and develop Syncthing Tray, how to add translations and debug
certain features.

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities),
[qtutilities](https://github.com/Martchus/qtutilities) and
[qtforkawesome](https://github.com/Martchus/qtforkawesome) and is built the same way as these libraries.
For basic instructions and platform-specific details checkout the README file of
[c++utilities](https://github.com/Martchus/cpp-utilities).

To avoid building c++utilities/qtutilities/qtforkawesome separately, follow the instructions under
"[Building this straight](#building-this-straight)". There's also documentation about
[various build variables](https://github.com/Martchus/cpp-utilities/blob/master/doc/buildvariables.md) which
can be passed to CMake to influence the build.

### Further dependencies
The following Qt modules are required (only the latest Qt 5 and Qt 6 version tested): Qt Core, Qt Concurrent,
Qt Network, Qt D-Bus, Qt Gui, Qt Widgets, Qt Svg, Qt WebEngineWidgets/WebKitWidgets

It is recommended to use at least Qt 5.14 to avoid limitations in previous versions (see
[the documentation on known bugs](known_bugs_and_workarounds.md) for details).

The built-in web view and therefore the modules WebEngineWidgets/WebKitWidgets are optional (see
section "[Select Qt module for web view and JavaScript](#select-qt-module-for-web-view-and-javascript)").

The Qt Quick UI needs at least Qt 6.9 and also additional Qt modules found in the Qt Declarative repository.
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
necessary to clean an existing build directly. All relevant parts will be re-built as necessary with the
new version.

---

It is also possible to build only the CLI (`syncthingctl`) by adding `-DNO_MODEL:BOOL=ON` and
`-DNO_FILE_ITEM_ACTION_PLUGIN:BOOL=ON` to the CMake arguments. Then only the Qt modules `core`,
`network` and `dbus` are required.

---

Building the testsuite requires CppUnit and Syncthing itself. Tests will spawn (and eventually terminate)
a test instance of Syncthing that does not affect a possibly existing Syncthing setup on the build host.

### Further build-time configuration
The systemd integration can be explicitly enabled/disabled at compile time by adding
`-DSYSTEMD_SUPPORT=ON/OFF` to the CMake arguments. If the systemd integration does not work be sure your
version of Syncthing Tray has been compiled with systemd support.

Note for distributors: There will be no hard dependency to systemd in any case. Distributions supporting
alternative init systems do *not* need to provide differently configured versions of Syncthing Tray.
Disabling the systemd integration is mainly intended for systems which do not use systemd at all (e.g.
Windows and MacOS).

---

It is possible to build Syncthing itself as a library as part of Syncthing Tray and configure its
Syncthing launcher to make use of this "built-in" version as an alternative way of launching Syncthing.
The build Syncthing and use of it in the launcher can be enabled by adding `-DNO_LIBSYNCTHING=OFF` and
`-DUSE_LIBSYNCTHING=ON` to the CMake arguments respectively. When building Syncthing itself a Go build
environment is required.

---

The updater can be explicitly enabled/disabled by adding `-DSETUP_TOOLS=ON/OFF` to the CMake arguments
when compiling `qtutilities` and `syncthingtray`. When enabled, `c++utilities` needs to compiled with
`-DUSE_LIBARCHIVE=ON` and `libarchive` becomes a dependency. With the built-in version of Syncthing
disabled this will also lead to a hard dependency on the crypo library of OpenSSL for signature
verification. (With the built-in version of Syncthing enabled the Go-based crypto code from Syncthing
itself is used instead of OpenSSL.)

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

## Translations
Currently translations for English and German are available. Qt's built-in localization/translation
framework is used under the hood.

Note that `syncthingctl` has not been internationalized yet so it supports only English.

### Add a new locale
Translations for further locales can be added quite easily:

1. Append a new translation file for the desired locale to the `TS_FILES` list
   in `connector/CMakeLists.txt`, `model/CMakeLists.txt`, `widgets/CMakeLists.txt`,
   `fileitemactionplugin/CMakeLists.txt`, `plasmoid/CMakeLists.txt` and
   `tray/CMakeLists.txt`.
2. Configure a new build, e.g. follow steps under *[Building this straight](#building-this-straight)*.
3. Conduct a full build or generate only translation files via the `translations` target.
4. New translation files should have been created by the build system under
   `connector/translations`, `model/translations`, `widgets/translations`,
   `fileitemactionplugin/translations`, `plasmoid/translations` and
   `tray/translations` and the `translations` folder of `qtutilities`.
5. Open the files with Qt Linguist to add translations. Qt Linguist is part of
   the [Qt Tools repository](http://code.qt.io/cgit/qt/qttools.git/) and its usage
   is [well documented](http://doc.qt.io/qt-5/linguist-translators.html).
6. If you have added translations for the mobile UI as well, it makes sense to add an entry
   for the new locale also in `tray/android/res/xml/locale_config.xml`. This is required
   for the locale to be selectable in app-specific language settings on Android.

### Extend/update existing translations
* For English, update the corresponding string literals within the source code.
* If necessary, sync the translation files with the source code like in step `2.`/`3.` of
  "[Add a new locale](#add-a-new-locale)". Check that no translations have been lost (except ones which are no
  longer required of course).
* Change the strings within the translation files found within the `translations`
  directories like in step `4.`/`5.` of "[Add a new locale](#add-a-new-locale)".

### Remarks
* Syncthing Tray displays also text from [qtutilities](https://github.com/Martchus/qtutilities).
  Hence it makes sense adding translations there as well (following the same procedure).
* The CLI `syncthingctl` currently does not support translations.

## Using backend libraries
The contained backend libraries (which provide connecting to Syncthing, data models and more) are written for internal
use within the components contained by this repository.

Hence those libraries do *not* provide a stable ABI/API. If you like to
use them to develop Syncthing integration or tooling with Qt and C++, it makes most sense to contribute it as an additional component
directly to this repository. Then I will be able to take it into account when changing the API.

## KDE integration
Since the Dolphin integration and the Plasmoid are plugins, testing and debugging requires a few extra steps.
See [Testing and debugging Dolphin/KIO plugin with Qt Creator](/fileitemactionplugin/testing.md)
and [Testing and debugging Plasmoid with Qt Creator](/plasmoid/testing.md).

## Logging
It is possible to turn on logging of the underlying library by setting environment variables:

* `LIB_SYNCTHING_CONNECTOR_LOG_ALL`: log everything mentioned in points below
* `LIB_SYNCTHING_CONNECTOR_LOG_API_CALLS`: log calls to Syncthing's REST-API
* `LIB_SYNCTHING_CONNECTOR_LOG_API_REPLIES`: log replies from Syncthing's REST-API (except events)
* `LIB_SYNCTHING_CONNECTOR_LOG_EVENTS`: log events emitted by Syncthing's events REST-API endpoint
* `LIB_SYNCTHING_CONNECTOR_LOG_DIRS_OR_DEVS_RESETTED`: log when folders/devices are internally reset
* `LIB_SYNCTHING_CONNECTOR_LOG_NOTIFICATIONS`: log computed high-level notifications/events
* `LIB_SYNCTHING_CONNECTOR_LOG_CERT_LOADING`: log loading of the (self-signed) certificate
* `SYNCTHINGTRAY_LOG_JS_CONSOLE`: log message from the JavaScript console of the built-in web view

On Windows, you'll have to use the `syncthingtray-cli` executable to see output in the terminal.

## Useful environment variables for development
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
