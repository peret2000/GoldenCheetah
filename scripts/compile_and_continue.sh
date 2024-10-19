#!/bin/bash

# Script para ganerar AppImage y extraer su contenido, cuando sólo se necesita compilar cambios
# Compila los cambios y genera AppImage

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

salida() {
	[[ -n "$1" && "$1" != "0" ]] && echo ">>>EJECUCIÓN FALLIDA: $1" | tee -a $LOGFILE
        scripts/pushover_end_compile.sh "Compile and Continue $HOSTNAME" $LOGFILE > /dev/null 2>&1
	echo Termina: `date` | tee -a $LOGFILE
        cat $LOGFILE >> $CUMLOGFILE
        rm $LOGFILE
        exit $1
}

export SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

export LOGFILE=$SCRIPT_DIR/logtmp.txt
export CUMLOGFILE=$SCRIPT_DIR/log.txt

export BUILDLOG=$SCRIPT_DIR/buildlog.txt
echo Comienzo: `date` > $BUILDLOG
echo ------------------------- >> $BUILDLOG

echo ------------------------- | tee $LOGFILE
echo Compilación sólo!!!!! | tee -a $LOGFILE
echo Comienzo: `date` | tee -a $LOGFILE

cd $SCRIPT_DIR/..

./travis/linux/script.sh >> $BUILDLOG 2>&1 && { echo "Compile OK" | tee -a $LOGFILE; } || { ERR=$?; echo "ERROR: Compile FAILED" | tee -a $LOGFILE; salida $ERR; }

echo ------------------------- >> $BUILDLOG
echo Finalización: `date` >> $BUILDLOG

# Generate the AppImage

echo after_success.sh: `date` | tee -a $LOGFILE

sed -i '/free.keep.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
sed -i 's/sudo //' travis/linux/after_success.sh
sed -i 's/git log -1 >> GCversionLinux.txt/git merge-base HEAD  goldencheetah\/master |xargs git log -1>>GCversionLinux.txt/' travis/linux/after_success.sh

[[ -d src/appdir ]] && rm -rf src/appdir
[[ -d squashfs-root ]] && rm -rf squashfs-root

[[ -f src/GoldenCheetah_v3.7-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.7-DEV_x64.AppImage
travis/linux/after_success.sh > /dev/null 2>&1 && { echo "deploy OK" | tee -a $LOGFILE; } || { ERR=$?; echo "ERROR: deploy FAILED" | tee -a $LOGFILE; salida $ERR; }

src/GoldenCheetah_v3.7-DEV_x64.AppImage --appimage-extract > /dev/null 2>&1

salida 0
