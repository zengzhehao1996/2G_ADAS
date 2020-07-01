#!/bin/bash
#######################################
## Liufeng
#######################################
	. ../zephyr-env.sh
CURR_DIR={PWD}
export SMART_BUILD_BOARD="SMART_LINK_V131"
export CUR_TIMESTAMP=`date +%s`
export SERVER_SWITCH=0
export LOG_SWITCH=1
export FOTA_SWITCH=0

OBJECT_BIN="zephyr.bin"
FOTA_OBJECT_BIN="fota-zephyr.bin"

die() {
	echo ERROR: "$@"
	exit 1
}

mix_bin_name(){
    CUR_DATE=`date +%y%m%d`
    version="0"
    MIDSTR=""
    tmp_file=.tmp_version
    if [ -f $tmp_file ]; then
        read version < $tmp_file
        rm -rf $tmp_file
        if [ $SERVER_SWITCH == 1 ] && [ $LOG_SWITCH == 1 ]; then
            MIDSTR="-insiders"
        elif [ $SERVER_SWITCH == 1 ] && [ $LOG_SWITCH == 0 ]; then
            MIDSTR="-stable"
        elif [ $SERVER_SWITCH == 0 ] && [ $LOG_SWITCH == 1 ]; then
            MIDSTR="-debug"
        else
            MIDSTR="-debug-logoff"
        fi
         
        if [ ${FOTA_SWITCH} -eq 1 ]; then
            OBJECT_BIN=$CUR_DATE-OnlyTest$MIDSTR-v$version.bin
            FOTA_OBJECT_BIN=$CUR_DATE-OnlyTest-FOTA$MIDSTR-v$version.bin
        else
            OBJECT_BIN=$CUR_DATE$MIDSTR-v$version.bin
            FOTA_OBJECT_BIN=$CUR_DATE-FOTA$MIDSTR-v$version.bin
        fi
    else
        die "Can't read version"
    fi
}

ARGS=`getopt -o hsidf --long help,stable,insiders,debug,fota,logoff -- "$@"`
if [ $? != 0 ] ; then echo "Attribute in \"-s | -i | -d | -f | --logoff\"" >&2 ; exit 1 ; fi
eval set -- "$ARGS"
while true;do
    case "$1" in
        -s|--stable)
            echo "-s | --stable"
            SERVER_SWITCH=1
            LOG_SWITCH=0
            shift 1
            ;;
        -i|--insiders)
            echo "-i | --insiders"
            SERVER_SWITCH=1
            LOG_SWITCH=1
            shift 1
            ;;
        -d|--debug)
            echo "-d | --debug"
            SERVER_SWITCH=0
            LOG_SWITCH=1
            shift 1
            ;;
        -f|--fota)
            echo "-f | --fota"
            FOTA_SWITCH=1
            shift 1
            ;;
        --logoff)
            echo "--logoff"
            LOG_SWITCH=0
            shift 1
            ;;
        -h|--help)
            echo "./para.sh [-s|-i|-d]"
            shift 1
            ;;
        --)
            shift
            break
            ;;
        *) 
            echo "Unknown attribute:{$1}"
            echo "Attribute in \"-s | -i | -d | -f | --logoff\""
            exit 1
            ;;
    esac
done


if [ -d out ]; then
	rm -rf out
fi
	mkdir out

if [ ! -f linde/lib/can/libcan.a ]; then
    die "Don't find libcan.a"
fi

    echo "server $SERVER_SWITCH"
    echo "log    $LOG_SWITCH"
    echo "fota   $FOTA_SWITCH"

# build mcuboot
if [ ! -f mcuboot-v13.bin ]; then
	BOARD=aidong_linde429v13 ./bin/build-boot.sh &&\
		./bin/alg128k mcuboot.bin || die "Build mcuboot"
	mv mcuboot.bin mcuboot-v13.bin
	echo "Build mcuboot OK!"
fi

# build linde_v133
	BOARD=aidong_linde429v13 DEMO=linde ./bin/build-demo.sh || die "Build
	linde_v131 failed" 
	echo "Build signed-linde_v131 OK!"

# check mcuboot
	cat mcuboot-v13.bin signed-zephyr.bin > zephyr.bin || die "Synthetic bin
	packet error!"
	echo "Synthetic bin packet OK!"

# crc zephyr.bin to 384k
	./bin/alg384k zephyr.bin  || die "Increase zephyr.bin error"
	echo "Increase zephyr.bin OK!"

# mix bin name
    mix_bin_name

if [ -d out ]; then
	#cp mcuboot-v13.bin out
	mv zephyr.bin out/$OBJECT_BIN
	mv signed-zephyr.bin out/$FOTA_OBJECT_BIN
	echo "The Brush Pack is generated in the ./out dirctory----"
fi

if [ "lux" = `whoami` ];then
	cp out/$OBJECT_BIN /mnt/hgfs/share/ && echo "COPY bin to share/ OK!" || die "Copy to share/ FAILED !!!"
fi
