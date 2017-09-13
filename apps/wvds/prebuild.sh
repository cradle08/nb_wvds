#!/bin/sh

verdir=../..
commit=$(git log -n 1 --pretty=format:'%h' $verdir)
build=$(git log -n 1 --pretty=format:'%ai' $verdir | sed -e 's/ .*//' -e 's/[-]//g')

sed -i -e "/#define APP_COMMIT/s/\".*\"/\"${commit}\"/" -e "/#define APP_BUILD/s/\".*\"/\"${build}\"/" app.h
