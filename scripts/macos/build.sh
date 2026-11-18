#!/usr/bin/env bash
# Builds Syncthing Tray on macOS (Apple Silicon or Intel) using Homebrew dependencies.
#
# Usage: scripts/macos/build.sh [deps|configure|build|test|bundle|all]   (default: all)
#
# Environment:
#   WORK_DIR    where dependency sources and the build tree go (default: <repo>/.macos)
#   BUILD_TYPE  CMake build type (default: RelWithDebInfo)
#   QT_PREFIX   Qt 6 installation prefix (default: $(brew --prefix qt))
#   CMAKE_EXTRA additional CMake arguments
set -euo pipefail

REPO_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
WORK_DIR=${WORK_DIR:-$REPO_DIR/.macos}
DEPS_DIR=$WORK_DIR/deps
BUILD_DIR=$WORK_DIR/build
INSTALL_DIR=$WORK_DIR/install
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}
QT_PREFIX=${QT_PREFIX:-$(brew --prefix qt)}
STEP=${1:-all}

deps() {
    brew list --versions qt boost cmake ninja cppunit >/dev/null || brew install qt boost cmake ninja cppunit
    mkdir -p "$DEPS_DIR"
    local repo name
    while read -r repo name; do
        if [[ -d $DEPS_DIR/$name/.git ]]; then
            git -C "$DEPS_DIR/$name" reset -q --hard  # drop previously applied patches
            git -C "$DEPS_DIR/$name" pull -q --ff-only
        else
            git clone -q --depth 1 "https://github.com/$repo.git" "$DEPS_DIR/$name"
        fi
        for patch in "$REPO_DIR/scripts/macos/patches/$name"-*.patch; do
            if [[ -e $patch ]]; then git -C "$DEPS_DIR/$name" apply "$patch"; fi
        done
    done <<EOF
Martchus/cpp-utilities c++utilities
Martchus/qtutilities qtutilities
Martchus/qtforkawesome qtforkawesome
ForkAwesome/Fork-Awesome forkawesome
Martchus/subdirs subdirs
EOF
    # subdirs expects syncthingtray next to the other repositories
    ln -sfn "$REPO_DIR" "$DEPS_DIR/syncthingtray"
}

configure() {
    cmake -S "$DEPS_DIR/subdirs/syncthingtray" -B "$BUILD_DIR" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DCMAKE_PREFIX_PATH="$QT_PREFIX;$(brew --prefix)" \
        -DQT_PACKAGE_PREFIX:STRING=Qt6 \
        -DKF_PACKAGE_PREFIX:STRING=KF6 \
        -DBUILTIN_TRANSLATIONS:BOOL=ON \
        -DBUILTIN_TRANSLATIONS_OF_QT:BOOL=OFF \
        -DSYSTEMD_SUPPORT:BOOL=OFF \
        -DBUILD_TESTING:BOOL=ON \
        `# Homebrew's Qt lacks a CMake package for the compile-time macOS style; select it at runtime` \
        -DQUICK_GUI_CONTROLS_STYLE:STRING=dynamic \
        -DBUNDLED_QT_FORK_AWESOME_PATH="$DEPS_DIR/qtforkawesome" \
        -DFORK_AWESOME_FONT_FILE="$DEPS_DIR/forkawesome/fonts/forkawesome-webfont.woff2" \
        -DFORK_AWESOME_ICON_DEFINITIONS="$DEPS_DIR/forkawesome/src/icons/icons.yml" \
        ${CMAKE_EXTRA:-}
}

build() {
    cmake --build "$BUILD_DIR"
}

run_tests() {
    # tests needing a running Syncthing are skipped automatically if no syncthing binary is in PATH
    # only Syncthing Tray's own test suites; c++utilities' tests don't compile against macOS' int64_t
    local targets
    targets=$(ninja -C "$BUILD_DIR" -t targets | sed -n 's/^\(syncthing[a-z]*_[a-z_]*tests\): phony$/\1/p')
    # shellcheck disable=SC2086
    ninja -C "$BUILD_DIR" $targets
    # render GUI tests offscreen so no windows/dialogs pop up
    QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD_DIR" --output-on-failure -R '^syncthing' 
}

bundle() {
    cmake --install "$BUILD_DIR"
    local app
    app=$(find "$INSTALL_DIR" -maxdepth 3 -name 'syncthingtray*.app' -print -quit)
    if [[ -z $app ]]; then
        echo "No .app bundle found under $INSTALL_DIR" >&2
        exit 1
    fi
    "$QT_PREFIX/bin/macdeployqt" "$app" -always-overwrite -libpath="$(brew --prefix)/lib"
    # macdeployqt leaves the install name of some Homebrew libraries pointing into the Homebrew prefix
    local lib
    for lib in "$app"/Contents/Frameworks/*.dylib; do
        if otool -D "$lib" | tail -n +2 | grep -q "^$(brew --prefix)/"; then
            install_name_tool -id "@rpath/$(basename "$lib")" "$lib" 2>/dev/null
        fi
    done
    # macdeployqt doesn't fix up the Qt WebEngine helper process, so let it use the bundled Qt frameworks
    local helper=$app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess
    if [[ -f $helper ]]; then
        local dep
        for dep in $(otool -L "$helper" | awk -v prefix="$(brew --prefix)/" 'index($1, prefix) == 1 { print $1 }'); do
            install_name_tool -change "$dep" "@rpath/${dep#*/lib/}" "$helper" 2>/dev/null
        done
        install_name_tool -add_rpath @loader_path/../../../../../../.. "$helper" 2>/dev/null || true
    fi
    # ad-hoc sign so Gatekeeper lets the locally built bundle run
    codesign --force --deep --sign - "$app"
    echo "App bundle: $app"
}

case $STEP in
    deps) deps ;;
    configure) configure ;;
    build) build ;;
    test) run_tests ;;
    bundle) bundle ;;
    all) deps; configure; build; run_tests; bundle ;;
    *) echo "Unknown step: $STEP" >&2; exit 1 ;;
esac
