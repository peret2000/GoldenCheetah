#!/bin/bash

# Argument (optional): branch to merge with


# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

salida() {
  scripts/pushover_end_compile.sh "Nightly Build" $LOGFILE
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
echo Comienzo: `date` >> $LOGFILE
echo git fetch, merge, etc >> $LOGFILE

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

git merge --no-edit origin/TrainButtons         && echo "merge TrainButtons OK" >> $LOGFILE || echo "merge TrainButtons FAILED" >> $LOGFILE
###### Cambio temporal
git merge --no-edit origin/MyZEWPR4400			&& echo "merge MyZEW OK" >> $LOGFILE  || echo "merge MyZEW FAILED" >> $LOGFILE
git merge --no-edit origin/VideoWidgets         && echo "merge VideoWidgets OK" >> $LOGFILE || echo "merge VideoWidgets FAILED" >> $LOGFILE
git merge --no-edit origin/SmoothPowerEstim	&& echo "merge SmoothPowerEstim OK" >> $LOGFILE  || echo "merge SmoothPowerEstim FAILED" >> $LOGFILE
git merge --no-edit origin/PythonScripts	&& echo "merge PythonScripts OK" >> $LOGFILE  || echo "merge PythonScripts FAILED" >> $LOGFILE
git merge --no-edit origin/Strava		&& echo "merge Strava OK" >> $LOGFILE  || echo "merge Strava FAILED" >> $LOGFILE

git merge --no-edit origin/PyAutomatedProcessors && echo "merge PyAutomatedProcessors OK" >> $LOGFILE  || echo "merge PyAutomatedProcessors FAILED" >> $LOGFILE

git merge --no-edit goldencheetah/master	&& echo "merge master OK" >> $LOGFILE  || echo "merge master FAILED" >> $LOGFILE

##### Cambio temporal
git remote add thejockl https://github.com/thejockl/GoldenCheetah.git
git merge --no-edit thejockl/411-feature-request-favorite-workouts-v2	&& echo "merge PR4400 OK" >> $LOGFILE  || echo "merge PR4400 FAILED" >> $LOGFILE


if [ "$1" ]; then
  git merge --no-edit origin/$1        		&& echo "merge " $1 " OK" >> $LOGFILE  || echo "merge " $1 " FAILED" >> $LOGFILE
fi

echo preparedirectory.sh: `date` >> $LOGFILE

./scripts/preparedirectory.sh  && { echo "preparedirectory OK" >> $LOGFILE; } || { ERR=$?; echo "preparedirectory FAILED" >> $LOGFILE; salida $ERR; }

echo script.sh: `date` >> $LOGFILE

# Ésta es una forma 'compleja' de ejecutar un comando, que muestre la salida por pantalla, además de escribir en un fichero, y utilizar
# el código de error de la salida (process substitution)
./travis/linux/script.sh > >(tee -a $BUILDLOG) 2> >(tee -a $BUILDLOG >&2) && { echo "Compile OK"; } || { ERR=$?; echo "Compile FAILED"; }
echo ------------------------- >> $BUILDLOG
echo Finalización: `date` >> $BUILDLOG

# Generate the AppImage

echo after_success.sh: `date` >> $LOGFILE

sed -i '/free.keep.sh/s/^/echo Commented out:/' travis/linux/after_success.sh 
sed -i 's/sudo //' travis/linux/after_success.sh
sed -i 's/git log -1 >> GCversionLinux.txt/git merge-base HEAD  goldencheetah\/master |xargs git log -1>>GCversionLinux.txt/' travis/linux/after_success.sh

[[ -d src/appdir ]] && rm -rf src/appdir
[[ -d squashfs-root ]] && rm -rf squashfs-root

[[ -f src/GoldenCheetah_v3.6-DEV_x64.AppImage ]] && rm src/GoldenCheetah_v3.6-DEV_x64.AppImage
travis/linux/after_success.sh				&& { echo "deploy OK" >> $LOGFILE; } || { ERR=$?; echo "deploy FAILED" >> $LOGFILE; salida $ERR; }


src/GoldenCheetah_v3.6-DEV_x64.AppImage --appimage-extract

echo Termina: `date` >> $LOGFILE

salida 0
