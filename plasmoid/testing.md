# Testing and debugging Plasma 5 plasmoid with Qt Creator

The following instructions allow to test the Plasmoid by installing it in a test directory
rather than the regular home to separate testing from production.

1. Build as usual, ensure `NO_PLASMOID` is turned off
2. Add build step to execute custom target `init_plasmoid_testing` which
   will install the Plasmoid in a test directory which is "$BUILD_DIR/plasmoid/testdir"
   by default
3. Add new config for run in Qt Creator and set `plasmoidviewer` (or `plasmawindowed`)
   as executable
4. In execution environment, set
  * `QT_PLUGIN_PATH` to directory containing plugin `\*.so`-file
  * `QT_DEBUG_PLUGINS` to 1 for verbose plugin detection
  * `HOME` to the test directory from step 2 so plasmoidviewer finds the Plasmoid
    in the test directory
5. Set `--applet martchus.syncthingplasmoid` as CLI argument
6. Ignore warning that executable is no debug build, it is sufficiant when
   the plugin is a debug build (see next section for QML debugging)

## Enable QML debugging

To enable QML debugging, it is required to rebuild `plasmoidviewer` with QML debugging
enabled.

1. Get plasma-sdk: `git clone https://anongit.kde.org/plasma-sdk.git`
2. Create a debug build of `plasmoidviewer` and ensure `QT_QML_DEBUG` is defined when
   compiling `plasmoidviewer`, eg. by adding the following lines to its project file:
   ```
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
       target_compile_definitions(plasmoidviewer PRIVATE -DQT_QML_DEBUG)
   endif()
   ```
3. Prepend the build directory containing the `plasmoidviewer` binary to the path variable
   in the build environment of Syncthing Tray.
4. Enable QML debugging in the *Run* section.
