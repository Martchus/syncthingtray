#!/bin/bash
set -e
script_dir=$(dirname "${BASH_SOURCE[0]}")
source "$script_dir/settestenv.sh"

# use the package dir within the source-tree so one does not need to run CMake again for updating
# build-tree copy all the time
package_dir=$script_dir/../$2

# copy the generated desktop file back into the source-tree package dir so it can actually be used
meta_data_file=$1
cp --target-directory="$package_dir" "$meta_data_file"

# install or update the package into the working directory
if ! plasmapkg2 --install "$package_dir"; then
    plasmapkg2 --upgrade "$package_dir"
fi
exit $?
