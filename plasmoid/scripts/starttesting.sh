#!/bin/bash
set -e
script_dir=$(dirname "${BASH_SOURCE[0]}")
source "$script_dir/settestenv.sh"
exec "$@"
