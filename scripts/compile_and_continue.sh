#!/bin/bash

# Script para ganerar AppImage y extraer su contenido, cuando sólo se necesita compilar cambios
# Compila los cambios y genera AppImage

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi


export SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

export LOGFILE=$SCRIPT_DIR/log.txt

cd $SCRIPT_DIR

echo ------------------------- >> $LOGFILE
echo Compilación sólo!!!!! >> $LOGFILE
echo Comienzo: `date` >> $LOGFILE

cd GoldenCheetah_Debug

make -j$(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)   && echo "Compile OK" >> $LOGFILE || echo "Compile FAILED" >> $LOGFILE

# Generate the AppImage

echo after_success.sh: `date` >> $LOGFILE

sed -i '/free.keep.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
sed -i 's/sudo //' travis/linux/after_success.sh

[[ -d src/appdir ]] && rm -rf src/appdir
[[ -d squashfs-root ]] && rm -rf squashfs-root

[[ -f src/GoldenCheetah_v3.6-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.6-DEV_x64.AppImage
travis/linux/after_success.sh                           && echo "deploy OK" >> $LOGFILE || echo "deploy FAILED" >> $LOGFILE

src/GoldenCheetah_v3.6-DEV_x64.AppImage --appimage-extract

