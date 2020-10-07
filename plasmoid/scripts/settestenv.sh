#!/bin/bash
set -e

# use a sub directory within the build directory which is supposed to be $PWD
export HOME=${TEST_HOME:-$PWD/plasmoid-testing}
echo "HOME directory used for Plasmoid testing: $HOME"
mkdir -p "$HOME"

# unset XDG_DATA_HOME and XDG_CONFIG_HOME to use Qt's default relying on just HOME (see qtbase/src/corelib/io/qstandardpaths_unix.cpp for defaults)
export XDG_DATA_HOME=
export XDG_CONFIG_HOME=

# set QT_PLUGIN_PATH if it has not already been set
if ! [[ $QT_PLUGIN_PATH ]]; then
    if [[ -f $PWD/syncthingtray/plasmoid/lib/plasma/applets/libsyncthingplasmoid.so ]]; then
        export QT_PLUGIN_PATH=$PWD/syncthingtray/plasmoid/lib
    elif [[ -f $PWD/plasmoid/lib/plasma/applets/libsyncthingplasmoid.so ]]; then
        export QT_PLUGIN_PATH=$PWD/plasmoid/lib
    fi
    echo "QT_PLUGIN_PATH used for Plasmoid testing: $QT_PLUGIN_PATH"
fi
