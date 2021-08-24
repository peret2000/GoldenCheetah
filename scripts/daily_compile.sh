#!/bin/bash

# Argument (optional): branch to merge with


# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.bashrc
fi


export SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

export LOGFILE=$SCRIPT_DIR/log.txt

cd $SCRIPT_DIR

echo ------------------------- >> $LOGFILE
echo Comienzo: `date` >> $LOGFILE
echo git fetch, merge, etc >> $LOGFILE

###git clone git@github.com:peret2000/GoldenCheetah.git GoldenCheetah_Debug

cd GoldenCheetah_Debug

###git remote add goldencheetah https://github.com/GoldenCheetah/GoldenCheetah.git

# Esto no debería ser necesario si se hace un git clone, partiendo de cero
# Es por si el repositorio se quedó con un merge a medias, por ejemplo, por un conflicto
# Si no había conflicto, dará un error que se puede ignorar
git merge --abort

git fetch --all

# Estos ficheros se modifican en la compilación y pueden dar problemas al hacer merge
git checkout -- src/Resources/translations/
git checkout -- src/Core/Secrets.h

git checkout MyBuildAdapt
git merge

# Por si existe ya la rama, primero se elimina
git branch -D NightlyBuild
git checkout -b NightlyBuild

git merge --no-edit origin/TrainButtons
git merge --no-edit origin/deltaSlope
git merge --no-edit origin/MyZEW

git merge --no-edit goldencheetah/master

if [ "$1" ]; then
  git merge --no-edit origin/$1
fi


###mkdir -p D2XX
###travis/linux/before_install.sh
###rm libftd2xx-x86_64-1.3.6.tgz
###rm v0.1.1git1.tar.gz

echo before_script.sh: `date` >> $LOGFILE


# Aquí se debe poner la variables de entorno $GC_STRAVA_CLIENT_SECRET (o existir ya) si se quiere compilar con ella

travis/linux/before_script.sh

sed -i '/GC_VERSION/ d' src/gcconfig.pri
echo DEFINES += GC_VERSION=\"\\\\\\\"\\\\\(Debug\\ `git log -1  goldencheetah/master | sed -n 's/^commit *//p' | cut -c -7`\\\\\)\\\\\\\"\"  >> src/gcconfig.pri

sed -i '/CONFIG += debug/ d' src/gcconfig.pri
sed -i '/CONFIG += release/ d' src/gcconfig.pri
sed -i '/-O3/ d' src/gcconfig.pri

echo CONFIG += debug static >> src/gcconfig.pri


echo script.sh: `date` >> $LOGFILE

sed -i 's/-j4/-j1/' travis/linux/script.sh

travis/linux/script.sh

echo Termina: `date` >> $LOGFILE
