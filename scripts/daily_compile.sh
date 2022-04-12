#!/bin/bash

# Argument (optional): branch to merge with


# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
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
git checkout -- travis/linux/script.sh

git checkout MyBuildAdapt
git merge

# Por si existe ya la rama, primero se elimina
git branch -D NightlyBuild
git checkout -b NightlyBuild

#git merge --no-edit origin/TrainButtons	# Ya no es necesario,  se ha implementado en 7230e2873
git merge --no-edit origin/deltaSlope		&& echo "merge deltaSlope OK" >> $LOGFILE || echo "merge deltaSlope FAILED" >> $LOGFILE
git merge --no-edit origin/elevationGain	&& echo "merge elevationGain OK" >> $LOGFILE  || echo "merge elevationGain FAILED" >> $LOGFILE
git merge --no-edit origin/MyZEW		&& echo "merge MyZEW OK" >> $LOGFILE  || echo "merge MyZEW FAILED" >> $LOGFILE
git merge --no-edit origin/SmoothPowerEstim	&& echo "merge SmoothPowerEstim OK" >> $LOGFILE  || echo "merge SmoothPowerEstim FAILED" >> $LOGFILE
git merge --no-edit origin/PythonScripts	&& echo "merge PythonScripts OK" >> $LOGFILE  || echo "merge PythonScripts FAILED" >> $LOGFILE
git merge --no-edit origin/Strava		&& echo "merge Strava OK" >> $LOGFILE  || echo "merge Strava FAILED" >> $LOGFILE

git merge --no-edit origin/PyAutomatedProcessors && echo "merge PyAutomatedProcessors OK" >> $LOGFILE  || echo "merge PyAutomatedProcessors FAILED" >> $LOGFILE

git merge --no-edit goldencheetah/master 	&& echo "merge master OK" >> $LOGFILE  || echo "merge master FAILED" >> $LOGFILE


if [ "$1" ]; then
  git merge --no-edit origin/$1        		&& echo "merge " $1 " OK" >> $LOGFILE  || echo "merge " $1 " FAILED" >> $LOGFILE
fi


###mkdir -p D2XX
###travis/linux/before_install.sh
###rm libftd2xx-x86_64-1.3.6.tgz
###rm v0.1.1git1.tar.gz

echo before_script.sh: `date` >> $LOGFILE


# Aquí se debe poner la variables de entorno $GC_STRAVA_CLIENT_SECRET (o existir ya) si se quiere compilar con ella

travis/linux/before_script.sh			&& echo "before_script OK" >> $LOGFILE || echo "before_script FAILED" >> $LOGFILE

sed -i '/GC_VERSION/ d' src/gcconfig.pri
echo DEFINES += GC_VERSION=\"\\\\\\\"\\\\\(Debug\\ `git log -1  goldencheetah/master | sed -n 's/^commit *//p' | cut -c -7`\\\\\)\\\\\\\"\"  >> src/gcconfig.pri

sed -i '/CONFIG += debug/ d' src/gcconfig.pri
sed -i '/CONFIG += release/ d' src/gcconfig.pri
sed -i '/-O3/ d' src/gcconfig.pri

echo CONFIG += debug static >> src/gcconfig.pri


echo script.sh: `date` >> $LOGFILE

sed -i 's/-j4/-j1/' travis/linux/script.sh

travis/linux/script.sh				&& echo "script OK" >> $LOGFILE || echo "script FAILED" >> $LOGFILE

echo Termina: `date` >> $LOGFILE
