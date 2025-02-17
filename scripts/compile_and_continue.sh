#!/bin/bash

# Script que compila cambios. No se genera AppImage, salvo que se mande el parámetro --appimage
# En ese caso, se genera AppImage y se extrae su contenido

# Si no se manda ese parámetro, se asume que no es necesaria la creación de la estructura de directorios
# Se asume que ya hay un directorio squashfs-root donde se depositará el binario
# Si no existe ya ese directorio con todo el paquete de librerías, etc, necesario para la ejecución,
# el binario no se ejecutará correctamente en ese entorno, pero realmente no es necesario, si la finalidad es
# copiar el binario a otro lugar

# Si se manda el parámetro, se genera todo el directorio squashfs-root


# Chequea si se manda el parámetro --appimage
if [[ "$1" == "--appimage" ]]; then
        export APPIMAGE=1
else
        export APPIMAGE=0
fi

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

[[ -d src/appdir ]] && rm -rf src/appdir

if [[ $APPIMAGE -eq 0 ]]; then
        echo genera binario con linuxdeployqt: `date` | tee -a $LOGFILE
        # Download current version of linuxdeployqt
        cd src
        wget --no-verbose -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
        chmod a+x linuxdeployqt-continuous-x86_64.AppImage
        # Deploy to appdir
        mkdir -p appdir
        cp -p GoldenCheetah appdir/
        # Lightweight deploy
        ./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah -verbose=2 -exclude-libs=libqsqlmysql,libqsqlpsql,libnss3,libnssutil3,libxcb-dri3.so.0 \
                -unsupported-allow-new-glibc -no-translations -no-plugins -no-copy-copyright-files -no-strip
        mkdir -p ../squashfs-root && mv appdir/GoldenCheetah ../squashfs-root/
        # Cleanup
        rm linuxdeployqt-continuous-x86_64.AppImage
        rm -rf ./appdir

else
        # Generate the AppImage

        echo after_success.sh: `date` | tee -a $LOGFILE

        sed -i '/temp.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
        sed -i 's/sudo //' travis/linux/after_success.sh
        sed -i 's/git log -1 >> GCversionLinux.txt/git merge-base HEAD  goldencheetah\/master |xargs git log -1>>GCversionLinux.txt/' travis/linux/after_success.sh

        [[ -d squashfs-root ]] && rm -rf squashfs-root

        [[ -f src/GoldenCheetah_v3.7-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.7-DEV_x64.AppImage
        travis/linux/after_success.sh > /dev/null 2>&1 && { echo "deploy OK" | tee -a $LOGFILE; } || { ERR=$?; echo "ERROR: deploy FAILED" | tee -a $LOGFILE; salida $ERR; }

        src/GoldenCheetah_v3.7-DEV_x64.AppImage --appimage-extract > /dev/null 2>&1
fi

salida 0
