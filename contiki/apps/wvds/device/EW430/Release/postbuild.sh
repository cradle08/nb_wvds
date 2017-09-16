#!/bin/sh

ROOT=../../../../..
VERPY=$(cygpath -a $ROOT/tools/version.py)

if [ "x$WVDS_DIST" == "x" ]; then
  echo "请定义环境变量WVDS_DIST，应指向运维软件的固件目录，多目录以;分隔"
  exit 1
fi
echo "* WVDS_DIST: $WVDS_DIST"
DIST=$(echo $WVDS_DIST | sed -e 's/\([a-zA-Z]\):/\/cygdrive\/\1/g' -e 's/\\/\//g' -e 's/;/:/g')
#echo "DIST: $DIST"

if [ "x$WVDS_PROD" == "x" ]; then
  echo "请定义环境变量WVDS_PROD，应指向生产烧录的固件目录，仅支持单个目录"
  exit 1
fi
echo "* WVDS_PROD: $WVDS_PROD"
PROD=$(echo $WVDS_PROD | sed -e 's/\([a-zA-Z]\):/\/cygdrive\/\1/g' -e 's/\\/\//g' -e 's/;/:/g')
#echo "PROD: $PROD"

verdir=../../../../..
verfile=../../device.c
verno="$(sed -n 's/^#define APP_VER_MAJOR\s\+\([0-9]\+\).*/\1/p' $verfile).$(sed -n 's/^#define APP_VER_MINOR\s\+\([0-9]\+\).*/\1/p' $verfile).$(sed -n 's/^#define APP_VER_PATCH\s\+\([0-9]\+\).*/\1/p' $verfile)"
commit=$(git log -n 1 --pretty=format:'%h' $verdir)
build=$(git log -n 1 --pretty=format:'%ai' $verdir | sed -e 's/ .*//' -e 's/[-]//g' -e 's/^20//')
echo "-------------------"
echo " Version: $verno"
echo " Commit : $commit"
echo " Build  : $build"
echo "-------------------"

#chan=$(sed -n '/#define RF_CHANNEL/s/^#define RF_CHANNEL\s\+\([0-9]\+\)\s.*/\1/p' ${ROOT}/platform/mist-exp5438/contiki-conf.h)
chan=$(sed -n 's/^.*pib.radioChan\s*=\s*\([0-9]*\);.*/\1/p' ../../device.c)

mode=$(sed -n '/define\s\+.*_SPACE\s\+1/s/^#define\s\+\([A-Z]\+\)_SPACE\s\+1/\1/p' ../../VehicleDetection.h | tr A-Z a-z | sed 's/\s\+$//')

#YEAR=$(sed -n '/nib.devmac\[2\] = /s/^.*nib.devmac\[2\] = 0x\([0-9A-F]\+\);.*/\1/p' ../../device.c)
#MON=$(sed -n '/nib.devmac\[3\] = /s/^.*nib.devmac\[3\] = 0x\([0-9A-F]\+\);.*/\1/p' ../../device.c)
#SEQH=$(sed -n '/nib.devmac\[4\] = /s/^.*nib.devmac\[4\] = 0x\([0-9A-F]\+\);.*/\1/p' ../../device.c)
#SEQL=$(sed -n '/nib.devmac\[5\] = /s/^.*nib.devmac\[5\] = \(.*\);.*/\1/p' ../../device.c)
#if [ "$SEQL" == "node_id" ]; then SEQL="03"; else SEQL=$(echo $SEQL | cut -c 3-4); fi
#devno="${YEAR}${MON}${SEQH}${SEQL}"

#FILE=VD-${build}-${commit}-ch${chan}_${devno}
#FILE=VD-${build}-${commit}-ch${chan}
#FILE=VD-${build}-v${verno}-${commit}-ch${chan}
FILE=VD-${build}-v${verno}-${commit}-ch${chan}-${mode}
python ${ROOT}/tools/msp430-txt-merge Exe/device.txt Exe/boot.txt
IFS=':' read -a DIRS <<< "${DIST}"
for d in "${DIRS[@]}"; do
  cp Exe/device.txt ${d}/${FILE}.txt
  cp Exe/device-boot.txt ${d}/${FILE}_boot.txt
  echo "* Copied as ${d}/$FILE"
done

echo "* Copied as ${PROD}/device-boot.txt"
cp Exe/device-boot.txt ${PROD}/
cp Exe/device-boot.txt ${PROD}/${FILE}_boot.txt

mv Exe/device.txt Exe/device-${mode}.txt
mv Exe/device-boot.txt Exe/device-${mode}-boot.txt

echo "* Update version info in ${PROD}/version.csv"
verdate=$(git log -n 1 --pretty=format:'%ai' $verdir | sed 's/ .*//')
digest=$(sha256sum ${PROD}/device-boot.txt | awk '{print $1;}')
echo "-------------------"
cd ${PROD} && python $VERPY device-boot.txt ${verno} ${commit} ${verdate} ${digest}
echo "-------------------"

exit 0
