#!/bin/bash

export SCRIPT_DIR=`dirname $0`

export LOGFILE=$SCRIPT_DIR/log.txt

cd $SCRIPT_DIR

echo ------------------------- >> $LOGFILE
echo Comienzo: `date` >> $LOGFILE
echo git fetch, merge, etc >> $LOGFILE

###git clone git@github.com:peret2000/GoldenCheetah.git GoldenCheetah_Debug

cd GoldenCheetah_Debug

###git remote add goldencheetah https://github.com/GoldenCheetah/GoldenCheetah.git

git fetch --all

git checkuot MiVersion
git merge
# Por si existe ya la rama, primero se elimina
git branch -D NightlyBuild
git checkout -b NightlyBuild
git merge --no-edit goldencheetah/master


###mkdir -p D2XX
###travis/linux/before_install.sh


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
