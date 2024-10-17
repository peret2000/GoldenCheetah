#!/bin/bash

# Argument (optional): branch to merge with


# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

salida() {
  scripts/pushover_end_compile.sh "Nightly Build" $LOGFILE
  echo Termina: `date` | tee -a $LOGFILE
  cat $LOGFILE >> $CUMLOGFILE
  rm $LOGFILE
  exit $1
}


export SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

export LOGFILE=$SCRIPT_DIR/logtmp.txt
export CUMLOGFILE=$SCRIPT_DIR/log.txt

merge() {
git merge --no-edit $1 > /dev/null 2>&1 || {
        ERR=$? ; echo "ERROR $ERR: merge $1 FAILED" | tee -a $LOGFILE ; salida $ERR
        } && echo "merge $1 OK" | tee -a $LOGFILE
}

echo ------------------------- | tee $LOGFILE
echo Comienzo: `date` | tee -a $LOGFILE
echo git fetch, merge, etc | tee -a $LOGFILE

###cd $SCRIPT_DIR/../.. && git clone git@github.com:peret2000/GoldenCheetah.git GoldenCheetah

cd $SCRIPT_DIR/..

###git remote add goldencheetah https://github.com/GoldenCheetah/GoldenCheetah.git

# Esto no debería ser necesario si se hace un git clone, partiendo de cero
# Es por si el repositorio se quedó con un merge a medias, por ejemplo, por un conflicto
# Si no había conflicto, dará un error que se puede ignorar
git merge --abort

git fetch --all

# Estos ficheros se modifican en la compilación y pueden dar problemas al hacer merge
git checkout -- src/Resources/translations/
git checkout -- src/Core/Secrets.h
git checkout -- travis/linux/script.sh
git checkout -- travis/linux/after_success.sh

git checkout MyBuildAdapt
git merge

# Por si existe ya la rama, primero se elimina
git branch -D NightlyBuild
git checkout -b NightlyBuild


merge origin/TrainButtonsZ
merge origin/MyZEW
merge origin/VideoWidgets
merge origin/SmoothPowerEstim
merge origin/PythonScripts
merge origin/Strava
merge origin/PyAutomatedProcessors

merge goldencheetah/master

#### Merge temporal del PR4533: Equipment management feature tiled
git remote add paulj49457 https://github.com/paulj49457/GoldenCheetah.git
git fetch paulj49457
merge paulj49457/equipment_feature_tiled
##############################

if [ "$1" ]; then
  merge origin/$1
fi

echo preparedirectory.sh: `date` | tee -a $LOGFILE

./scripts/preparedirectory.sh  && { echo "preparedirectory OK" | tee -a $LOGFILE; } || { ERR=$?; echo "preparedirectory FAILED" | tee -a $LOGFILE; salida $ERR; }

echo script.sh: `date` | tee -a $LOGFILE

export BUILDLOG=$SCRIPT_DIR/buildlog.txt
echo Comienzo: `date` > $BUILDLOG
echo ------------------------- >> $BUILDLOG
# Ésta es una forma 'compleja' de ejecutar un comando, que muestre la salida por pantalla, además de escribir en un fichero, y utilizar
# el código de error de la salida (process substitution)
./travis/linux/script.sh > >(tee -a $BUILDLOG) 2> >(tee -a $BUILDLOG >&2) \
	&& { echo "Compile OK" | tee -a $LOGFILE;} || { ERR=$?; echo "Compile FAILED" | tee -a $LOGFILE; salida $ERR;}
echo ------------------------- >> $BUILDLOG
echo Finalización: `date` >> $BUILDLOG

# Generate the AppImage

echo after_success.sh: `date` | tee -a $LOGFILE

sed -i '/temp.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
sed -i 's/sudo //' travis/linux/after_success.sh
sed -i 's/git log -1 >> GCversionLinux.txt/git merge-base HEAD  goldencheetah\/master |xargs git log -1>>GCversionLinux.txt/' travis/linux/after_success.sh

[[ -d src/appdir ]] && rm -rf src/appdir
[[ -d squashfs-root ]] && rm -rf squashfs-root

[[ -f src/GoldenCheetah_v3.7-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.7-DEV_x64.AppImage
travis/linux/after_success.sh && { echo "deploy OK" | tee -a $LOGFILE; } || { ERR=$?; echo "deploy FAILED" | tee -a $LOGFILE; salida $ERR; }

src/GoldenCheetah_v3.7-DEV_x64.AppImage --appimage-extract

salida 0
