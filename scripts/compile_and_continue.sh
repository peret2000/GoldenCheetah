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

export BUILDLOG=$SCRIPT_DIR/buildlog.txt
echo Comienzo: `date` > $BUILDLOG
echo ------------------------- >> $BUILDLOG

echo ------------------------- > $LOGFILE
echo Compilación sólo!!!!! >> $LOGFILE
echo Comienzo: `date` >> $LOGFILE

cd $SCRIPT_DIR/..

# Ésta es una forma 'compleja' de ejecutar un comando, que muestre la salida por pantalla, además de escribir en un fichero, y utilizar
# el código de error de la salida (process substitution)
./travis/linux/script.sh > >(tee -a $BUILDLOG) 2> >(tee -a $BUILDLOG >&2) \
		&& { echo "Compile OK"; >> $LOGFILE;} || { ERR=$?; echo "Compile FAILED" >> $LOGFILE; salida $ERR;}
echo ------------------------- >> $BUILDLOG
echo Finalización: `date` >> $BUILDLOG

# Generate the AppImage

echo after_success.sh: `date` >> $LOGFILE

sed -i '/free.keep.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
sed -i 's/sudo //' travis/linux/after_success.sh
sed -i 's/git log -1 >> GCversionLinux.txt/git merge-base HEAD  goldencheetah\/master |xargs git log -1>>GCversionLinux.txt/' travis/linux/after_success.sh

[[ -d src/appdir ]] && rm -rf src/appdir
[[ -d squashfs-root ]] && rm -rf squashfs-root

[[ -f src/GoldenCheetah_v3.7-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.7-DEV_x64.AppImage
travis/linux/after_success.sh && { echo "deploy OK" >> $LOGFILE; } || { ERR=$?; echo "deploy FAILED" >> $LOGFILE; salida $ERR;}

src/GoldenCheetah_v3.7-DEV_x64.AppImage --appimage-extract

salida 0
