#!/bin/sh

ROOT=../../../../..
VERPY=$(cygpath -a $ROOT/tools/version.py)

if [ "x$WVDS_PROD" == "x" ]; then
  echo "请定义环境变量WVDS_PROD，应指向生产烧录的固件目录，多目录以;分隔"
  exit 1
fi
echo "* WVDS_PROD: $WVDS_PROD"
PROD=$(echo $WVDS_PROD | sed -e 's/\([a-zA-Z]\):/\/cygdrive\/\1/g' -e 's/\\/\//g' -e 's/;/:/g')
#echo "PROD: $PROD"

verdir=../../../../..
verfile=../../test.c
verno="$(sed -n 's/^#define APP_VER_MAJOR\s\+\([0-9]\+\).*/\1/p' $verfile).$(sed -n 's/^#define APP_VER_MINOR\s\+\([0-9]\+\).*/\1/p' $verfile).$(sed -n 's/^#define APP_VER_PATCH\s\+\([0-9]\+\).*/\1/p' $verfile)"
commit=$(git log -n 1 --pretty=format:'%h' $verdir)

echo "* Copied as ${PROD}/parking.txt"
cp Exe/parking.txt ${PROD}/

echo "* Update version info in ${PROD}/version.csv"
verdate=$(git log -n 1 --pretty=format:'%ai' $verdir | sed 's/ .*//')
digest=$(sha256sum ${PROD}/parking.txt | awk '{print $1;}')
echo "-------------------"
cd ${PROD} && python $VERPY parking.txt ${verno} ${commit} ${verdate} ${digest}
echo "-------------------"

exit 0
