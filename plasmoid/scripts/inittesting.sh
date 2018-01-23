#!/bin/sh
export HOME="$PWD"
plasmapkg2 --install $(dirname $0)/../package || plasmapkg2 --upgrade $(dirname $0)/../package
exit $?
