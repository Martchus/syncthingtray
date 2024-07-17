# Testing and debugging Dolphin/KIO plugin with Qt Creator
1. Build as usual, ensure `NO_FILE_ITEM_ACTION_PLUGIN` is turned off.
2. For KF5, copy the `*.desktop` file from the build directory to `~/.local/share/kservices5` and enable
   it in Dolphin. You may skip this step if a packaged version with the same configuration name is already
   installed. As of KF6 this step is not required anymore.
3. Add new config for run in Qt Creator and set `dolphin` as executable.
4. In the execution environment, set
    * `QT_PLUGIN_PATH` to the build directory of the plugin (for KF5 this directory contains the `so`-file
      directly, for KF6 it is in the sub directory `kf6/kfileitemaction`).
    * `QT_DEBUG_PLUGINS` to 1 for verbose plugin detection.
    * `QT_XCB_NO_GRAB_SERVER` to 1 to prevent grabbing so the debugger is usable (not required under
      Wayland).
5. Ignore the warning that the executable is no debug build. It is sufficient when the plugin is a debug
   build.

## Testing against a development build of Dolphin
1. Build the whole dependency chain up to `dolphin` installing it under some custom prefix.
2. Then follow the usual steps but make sure you build Syncthing Tray against the custom KDE builds.
   This is achieved the easiest by using the `debug-kde-custom` CMake preset. This preset uses the
   environment variable `KDE_INSTALL_DIR` which must point to the custom prefix used in step 1.
3. Source the `prefix.sh` script that should be present in the build directory of any KDE library
   you built in step 1, e.g. `source kde/plasma-sdk/prefix.sh`.
4. When setting the environment one needs to be more careful to not override variables set in step 3.
   It is the easiest to just start `dolphin` from the shell:
   ```
   QT_PLUGIN_PATH=$BUILD_DIR/syncthingtray/debug-kde-custom/syncthingtray/fileitemactionplugin:$QT_PLUGIN_PATH dolphin
   ```
