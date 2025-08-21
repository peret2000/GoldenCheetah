#!/bin/bash

# Parameters: [--appimage] [--updatecode] [--fromscratch] [--help|-h]

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

salida() {
	[[ -n "$1" && "$1" != "0" ]] && echo ">>>EJECUCIÓN FALLIDA: $1" | tee -a $LOGFILE
	scripts/pushover_end_compile.sh "$TEXT $HOSTNAME" $LOGFILE > /dev/null 2>&1
	echo Termina: `date` | tee -a $LOGFILE
	cat $LOGFILE >> $CUMLOGFILE
	rm $LOGFILE
	exit $1
}


export SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

export LOGFILE=$SCRIPT_DIR/logtmp.txt
export CUMLOGFILE=$SCRIPT_DIR/log.txt
export BUILDLOG=$SCRIPT_DIR/buildlog.txt

APPIMAGE=false
FROMSCRATCH=false
MERGECODE=false	# Si es from scratch, se ignora

# Script command line help
mostrar_ayuda() {
    echo "Usage: $0 [options]"
    echo ""
    echo "With no options, it will just compile incrementally the project, without updating the source code."
    echo "In order for the incremental build to work, the project must have been built from scratch at least once"
    echo "(From scratch makes necessary changes)"
    echo ""
    echo "Options:"
    echo "  --appimage      Creates the appimage file"
    echo "  --fromscratch   Builds from scratch"
    echo "  --updatecode    If not from scratch, this option updates source from repository"
    echo "  --help, -h      Shows this help message"
    echo ""
    exit 1
}

TEXT="Compilación incremental de GoldenCheetah"

# Procesamos los argumentos
while [[ $# -gt 0 ]]; do
    case "$1" in
        --appimage)
            APPIMAGE=true
            shift
            ;;
        --fromscratch)
            FROMSCRATCH=true
			MERGECODE=true
			TEXT="Compilación completa"
            shift
            ;;
		--updatecode)
			MERGECODE=true
            shift
            ;;
        --help|-h)
            mostrar_ayuda
            ;;
        *)
            echo "Opción desconocida: $1"
            mostrar_ayuda
            ;;
    esac
done


# $2 es opcional. Para hacer merge en una rama que no es la actual
merge() {
git merge --no-edit $1 $2 > /dev/null 2>&1 || {
        ERR=$? ; echo "ERROR $ERR: merge $1 FAILED" | tee -a $LOGFILE ; salida $ERR
        } && echo "merge $1 OK" | tee -a $LOGFILE
}

echo ------------------------- | tee $LOGFILE $BUILDLOG
echo Comienzo: `date` | tee -a $LOGFILE $BUILDLOG
echo $TEXT | tee -a $LOGFILE

###cd $SCRIPT_DIR/../.. && git clone git@github.com:peret2000/GoldenCheetah.git GoldenCheetah

cd $SCRIPT_DIR/..

BUILDBRANCH=MyBuildAdapt

if $FROMSCRATCH; then

	echo git fetch, merge, etc | tee -a $LOGFILE

	###git remote add goldencheetah https://github.com/GoldenCheetah/GoldenCheetah.git

	# Esto no debería ser necesario si se hace un git clone, partiendo de cero
	# Es por si el repositorio se quedó con un merge a medias, por ejemplo, por un conflicto
	# Si no había conflicto, dará un error que se puede ignorar
	git merge --abort > /dev/null 2>&1

	# Estos ficheros se modifican en la compilación y pueden dar problemas al hacer merge
	git checkout -- src/Resources/translations/
	git checkout -- src/Core/Secrets.h
	git checkout -- src/Resources/linux/MakeAppImageQt6.sh
	git checkout -- travis/linux/script.sh

	git checkout $BUILDBRANCH

	# Por si existe ya la rama, primero se elimina y luego se crea
	git branch -D NightlyBuild

fi	# if $FROMSCRATCH; then

# Siempre se actualiza la rama MyBuildAdapt. En caso de no estar en la última versión, se aborta el script
git fetch --all

# Chequea que esté en la última versión
COMMIT_BEFORE=$(git rev-parse $BUILDBRANCH)
COMMIT_AFTER=$(git rev-parse origin/$BUILDBRANCH)
if [ "$COMMIT_BEFORE" != "$COMMIT_AFTER" ]; then
	echo "FAILED. $BUILDBRANCH NOT in last version." | tee -a $LOGFILE
	salida $ERR
fi

if ! git checkout -B NightlyBuild; then
	echo "ERROR: Not able to switch to NightlyBuild. Maybe not in previously built directory" | tee -a $LOGFILE
	salida $ERR
fi

git merge $BUILDBRANCH || { ERR=$?; echo "Unable to merge $BUILDBRANCH, Maybe branch has diverged. Process FAILED." | tee -a $LOGFILE; salida $ERR; }

if $MERGECODE; then

	merge origin/TrainButtons
	merge origin/MyZEW
	merge origin/VideoWidgets
	merge origin/SmoothPowerEstim
	merge origin/Strava
	merge origin/PyAutomatedProcessors
	merge origin/treadmill_qdomyos
	merge origin/utils
	merge origin/train_view_improvements
	merge origin/activities_view_improvements
	merge origin/train_geolocation_widget

	merge goldencheetah/master

	#### Merge temporal: Equipment management feature tiled
	### Se deja de hacer merge con paulj49457/origin-equipment-management-feature
	### La rama ha sido borrada para hacer otro diseño
	###git remote add paulj49457 https://github.com/paulj49457/GoldenCheetah.git > /dev/null 2>&1
	###git fetch paulj49457
	merge origin/tmp-equipment-management-feature
	###merge paulj49457/origin-equipment-management-feature
	##############################

fi	# if $MERGECODE; then

if $FROMSCRATCH; then
	echo preparedirectory.sh: `date` | tee -a $LOGFILE
	./scripts/preparedirectory.sh > /dev/null 2>&1 && { echo "preparedirectory OK" | tee -a $LOGFILE; } || { ERR=$?; echo "preparedirectory FAILED" | tee -a $LOGFILE; salida $ERR; }
fi	# if $FROMSCRATCH; then



echo script.sh: `date` | tee -a $LOGFILE

### Ésta es una forma 'compleja' de ejecutar un comando, que muestre la salida por pantalla, además de escribir en un fichero, y utilizar
### el código de error de la salida (process substitution)
##./travis/linux/script.sh > >(tee -a $BUILDLOG) 2> >(tee -a $BUILDLOG >&2) \
##	&& { echo "Compile OK" | tee -a $LOGFILE;} || { ERR=$?; echo "Compile FAILED" | tee -a $LOGFILE; salida $ERR;}
# No saca la salida por pantalla
./travis/linux/script.sh >> $BUILDLOG 2>&1 && { echo "Compile OK" | tee -a $LOGFILE; } || { ERR=$?; echo "ERROR: Compile FAILED" | tee -a $LOGFILE; salida $ERR; }

echo ------------------------- >> $BUILDLOG
echo Finalización: `date` >> $BUILDLOG

[[ -d src/appdir ]] && rm -rf src/appdir

if ! $APPIMAGE; then

	echo genera binario con linuxdeployqt: `date` | tee -a $LOGFILE
	# Download current version of linuxdeployqt
	cd src
	wget --no-verbose -c https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
	chmod a+x linuxdeployqt-continuous-x86_64.AppImage
	# Deploy to appdir
	mkdir -p appdir
	cp -p GoldenCheetah appdir/
	# Lightweight deploy
	./linuxdeployqt-continuous-x86_64.AppImage appdir/GoldenCheetah -verbose=2 -exclude-libs=libqsqlmysql,libqsqlpsql,libqsqlmimer,libqsqlodbc,libnss3,libnssutil3,libxcb-dri3.so.0 \
			-unsupported-allow-new-glibc -no-translations -no-plugins -no-copy-copyright-files -no-strip -qmake=/usr/bin/qmake6
	mkdir -p ../squashfs-root && mv appdir/GoldenCheetah ../squashfs-root/
	# Cleanup
	rm linuxdeployqt-continuous-x86_64.AppImage
	rm -rf ./appdir

else

	# Generate the AppImage

	echo MakeAppImageQt6.sh: `date` | tee -a $LOGFILE

	[[ -d squashfs-root ]] && rm -rf squashfs-root

	ls src/GoldenCheetah*.AppImage >/dev/null 2>&1 && rm src/GoldenCheetah*.AppImage
	cd src
	./Resources/linux/MakeAppImageQt6.sh > /dev/null 2>&1 && { echo "deploy OK" | tee -a $LOGFILE; } || { ERR=$?; echo "ERROR: deploy FAILED" | tee -a $LOGFILE; salida $ERR; }
	cd ..
	src/GoldenCheetah_v3.7_x64Qt6.AppImage --appimage-extract > /dev/null 2>&1

fi

salida 0
