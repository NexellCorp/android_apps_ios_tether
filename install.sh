#!/bin/bash

PATH_LOCAL="../../../../out/target/product"
PATH_LOCAL_BIN="../../../../hardware/samsung_slsi/slsiap/ios_tether"

function usage()
{
    echo "Usage: $0 -b <board-name> "
    echo -e ' -b <board-name> : target board name. ex) s5p4418_avn_ref, s5p6818_avn_ref \n\n'
}

function parse_args()
{
    TEMP=`getopt -o "b:h" -- "$@"`
    eval set -- "$TEMP"

    while true; do
        case "$1" in
            -b ) BOARD_NAME=$2; shift 2 ;;
            -h ) usage; exit 1 ;;
            -- ) break ;;
            *  ) echo -e '\ninvalid option'; usage; exit 1 ;;
        esac
    done
}


parse_args $@

adb root

sleep 5

echo "adb push $PATH_LOCAL/$BOARD_NAME/system/lib/libiOSMgr.so /system/lib/"
adb push $PATH_LOCAL/$BOARD_NAME/system/lib/libiOSMgr.so /system/lib/

echo "adb push $PATH_LOCAL/$BOARD_NAME/system/lib/libiOSevent.so /system/lib/"
adb push $PATH_LOCAL/$BOARD_NAME/system/lib/libiOSevent.so /system/lib/

echo "adb push $PATH_LOCAL/$BOARD_NAME/system/bin/ipod_dev_mgr_client /system/bin"
adb push $PATH_LOCAL/$BOARD_NAME/system/bin/ipod_dev_mgr_client /system/bin

echo "adb push $PATH_LOCAL/$BOARD_NAME/system/bin/ipod_dev_mgr_server /system/bin"
adb push $PATH_LOCAL/$BOARD_NAME/system/bin/ipod_dev_mgr_server /system/bin

echo "adb push $PATH_LOCAL/$BOARD_NAME/system/bin/usbmuxdd /system/bin"
adb push $PATH_LOCAL/$BOARD_NAME/system/bin/usbmuxdd /system/bin

cp -v $PATH_LOCAL/$BOARD_NAME/system/lib/libiOSevent.so $PATH_LOCAL_BIN/libiOSevent/lib/libiOSevent.so

