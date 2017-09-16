#!/bin/sh

set -x

verdir=.
commit=$(git log -n 1 --pretty=format:'%h' $verdir)
verdate=$(git log -n 1 --pretty=format:'%ai' $verdir | sed 's/ .*$//')
verfile=src/main/java/com/cadre/wvds/wvdsota/Version.java
sed -i -e "/build = /s/\".*\"/\"${verdate}\"/" -e "/commit = /s/\".*\"/\"${commit}\"/" ${verfile}

ant dist
