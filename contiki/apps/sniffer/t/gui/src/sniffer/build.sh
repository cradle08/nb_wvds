#!/bin/sh

set -x

commit=$(git log -n 1 src | head -n 1 | sed 's/^commit \([0-9a-f]\{7\}\).*/\1/')
verdate=$(git log -n 1 --pretty=format:'%ai' src | sed 's/ .*$//')
verfile=src/main/java/com/cadre/wvds/sniffer/Version.java

sed -i -e "/build = /s/\".*\"/\"$verdate\"/" -e "/commit = /s/\".*\"/\"$commit\"/" $verfile
#gradle jar
ant dist
