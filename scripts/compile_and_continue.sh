#!/bin/bash

# Script para ganerar AppImage y extraer su contenido, cuando sólo se necesita compilar cambios
# Compila los cambios y genera AppImage

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

salida() {
        scripts/pushover_end_compile.sh "Compile and Continue" $LOGFILE
        cat $LOGFILE >> $CUMLOGFILE
        rm $LOGFILE
        exit $1
}

export SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

export LOGFILE=$SCRIPT_DIR/logtmp.txt
export CUMLOGFILE=$SCRIPT_DIR/log.txt

echo ------------------------- > $LOGFILE
echo Compilación sólo!!!!! >> $LOGFILE
echo Comienzo: `date` >> $LOGFILE

cd $SCRIPT_DIR/..

make -j$(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l) && { echo "Compile OK" >> $LOGFILE; } || { ERR=$?; echo "Compile FAILED" >> $LOGFILE; salida $ERR; }

# Generate the AppImage

echo after_success.sh: `date` >> $LOGFILE

sed -i '/free.keep.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
sed -i 's/sudo //' travis/linux/after_success.sh

[[ -d src/appdir ]] && rm -rf src/appdir
[[ -d squashfs-root ]] && rm -rf squashfs-root

[[ -f src/GoldenCheetah_v3.6-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.6-DEV_x64.AppImage
travis/linux/after_success.sh && { echo "deploy OK" >> $LOGFILE; } || { ERR=$?; echo "deploy FAILED" >> $LOGFILE; salida $ERR;}

src/GoldenCheetah_v3.6-DEV_x64.AppImage --appimage-extract

salida 0
