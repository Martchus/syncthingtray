#!/bin/bash
script_dir=$(dirname "${BASH_SOURCE[0]}")
plasma_tools=(plasmapkg2 kpackagetool kpackagetool6 kpackagetool5)
plasma_tool=($(which "${plasma_tools[@]}" 2> /dev/null))
plasma_tool_args=()

set -e
if [[ "${#plasma_tool[0]}" -eq 0 ]]; then
    echo "No tool to install/update Plasmoids found. One of the following tools needs to be installed: ${plasma_tools[*]}"
    exit 1
fi

source "$script_dir/settestenv.sh"

# use the package dir within the source-tree so one does not need to run CMake again for updating
# build-tree copy all the time
package_dir=$script_dir/../$2

# copy the generated desktop file back into the source-tree package dir so it can actually be used
meta_data_file=$1
plasmoid_id=$(cat "$meta_data_file" | jq -r .KPlugin.Id)
echo "Plasmoid ID: $plasmoid_id"

if ! jq -e '.KPackageStructure' "$meta_data_file" > /dev/null 2>&1; then
    echo "Plasmoid is built as QML module; ensuring no stale package shadows it in $HOME/.local/share/plasma/plasmoids/$plasmoid_id"
    rm -rf "$HOME/.local/share/plasma/plasmoids/$plasmoid_id"
    exit 0
fi

cp -v --target-directory="$package_dir" "$meta_data_file"

# specify the package type when using kpackagetool
plasma_tool_name=${plasma_tool[0]##*/}
if [[ $plasma_tool_name =~ kpackagetool\d* ]]; then
    plasma_tool_args+=(--type Plasma/Applet)
fi

# install or update the package into the working directory
if ! "${plasma_tool[0]}" "${plasma_tool_args[@]}" --install "$package_dir"; then
    echo "Trying to upgrade existing Plasmoid instead"
    "${plasma_tool[0]}" "${plasma_tool_args[@]}" --upgrade "$package_dir"
fi
exit $?
