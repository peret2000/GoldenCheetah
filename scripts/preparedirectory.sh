#!/bin/bash

# IMPORTANTE: Ejecutar desde el directorio del repositorio: por ejemplo:
# ./scripts/preparedirectory.sh [debug]

# Deben estar las variables de entorno $GC_STRAVA_CLIENT_SECRET y otras que se utilizan
# para los servicios de cloud

# Prepara el directorio para compilar por primera vez

# Si se ejecuta el script diario (build_project.sh), ya prepara el directorio, pues llama a este script.
# Si no, es necesario ejecutar este script, para preparar una compilación, tanto de QtCreator como manual

# Si se manda como parámetro 'Debug' (case insensitive) prepara la compilación para debug.

# ATENCIÓN: Si se ha ejecutado en un directorio para debug (por ejemplo un 'build' de QtCreator) y se quiere continuar una compilación
# de 'release' en otro directorio (por ejemplo, una compilaqción manual), habrá que ejecutar de nuevo este script, para dejar
# gcconfig.pri como 'release' (y eso conlleva que tiene que compilar todo de nuevo)

# Modifica y Ejecuta appveyor/linux/before_build.sh
# Modifica gcconfig.pri

# Si se quiere usar un entorno de Qt diferente al por defecto (por ejemplo Qt6.6.1), debe existir la variable QT_DIR,
# y se usa para sustituir paths que hay hardcoded en los scripts de travis/linux

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

# Aquí se debe poner la variables de entorno $GC_STRAVA_CLIENT_SECRET (o existir ya) si se quiere compilar con ella

# Patch Secrets.h
sed -i "s/__GC_GOOGLE_CALENDAR_CLIENT_SECRET__/"$GC_GOOGLE_CALENDAR_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_STRAVA_CLIENT_SECRET__/"$GC_STRAVA_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_DROPBOX_CLIENT_SECRET__/"$GC_DROPBOX_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_CYCLINGANALYTICS_CLIENT_SECRET__/"$GC_CYCLINGANALYTICS_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_TWITTER_CONSUMER_SECRET__/"$GC_TWITTER_CONSUMER_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_DROPBOX_CLIENT_ID__/"$GC_DROPBOX_CLIENT_ID"/" src/Core/Secrets.h
sed -i "s/__GC_MAPQUESTAPI_KEY__/"$GC_MAPQUESTAPI_KEY"/" src/Core/Secrets.h
sed -i "s/__GC_CLOUD_DB_BASIC_AUTH__/"$GC_CLOUD_DB_BASIC_AUTH"/" src/Core/Secrets.h
sed -i "s/__GC_CLOUD_DB_APP_NAME__/"$GC_CLOUD_DB_APP_NAME"/" src/Core/Secrets.h
sed -i "s/__GC_GOOGLE_DRIVE_CLIENT_ID__/"$GC_GOOGLE_DRIVE_CLIENT_ID"/" src/Core/Secrets.h
sed -i "s/__GC_GOOGLE_DRIVE_CLIENT_SECRET__/"$GC_GOOGLE_DRIVE_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_GOOGLE_DRIVE_API_KEY__/"$GC_GOOGLE_DRIVE_API_KEY"/" src/Core/Secrets.h
sed -i "s/__GC_WITHINGS_CONSUMER_SECRET__/"$GC_WITHINGS_CONSUMER_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_NOKIA_CLIENT_SECRET__/"$GC_NOKIA_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_SPORTTRACKS_CLIENT_SECRET__/"$GC_SPORTTRACKS_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/OPENDATA_DISABLE/OPENDATA_ENABLE/" src/Core/Secrets.h
sed -i "s/__GC_CLOUD_OPENDATA_SECRET__/"$GC_CLOUD_OPENDATA_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_RWGPS_API_KEY__/"$GC_RWGPS_API_KEY"/" src/Core/Secrets.h
sed -i "s/__GC_NOLIO_CLIENT_ID__/"$GC_NOLIO_CLIENT_ID"/" src/Core/Secrets.h
sed -i "s/__GC_NOLIO_SECRET__/"$GC_NOLIO_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_XERT_CLIENT_SECRET__/"$GC_XERT_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_AZUM_CLIENT_SECRET__/"$GC_AZUM_CLIENT_SECRET"/" src/Core/Secrets.h
sed -i "s/__GC_TRAINERDAY_API_KEY__/"$GC_TRAINERDAY_API_KEY"/" src/Core/Secrets.h


# Directory where python3 is installed
PYTHONDIR="$(dirname "$(dirname "$(command -v python3)")")"
PYTHONVERS=$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
sed -i "s|.*PYTHONINCLUDES.*$|echo PYTHONINCLUDES = -I$PYTHONDIR/include/python$PYTHONVERS >> src/gcconfig.pri|" appveyor/linux/before_build.sh
sed -i "s|.*PYTHONLIBS.*$|echo PYTHONLIBS = -L$PYTHONDIR/lib -lpython$PYTHONVERS >> src/gcconfig.pri|" appveyor/linux/before_build.sh
appveyor/linux/before_build.sh || { ERR=$?; exit $ERR; }

# In case the binary remains from previous compilations, it is removed
rm -f src/GoldenCheetah

# Download necessary header file if it was not downloaded yet
# D2XX - refresh cache if folder is empty
mkdir -p D2XX
if [ -z "$(ls -A D2XX)" ]; then
    wget --no-verbose https://ftdichip.com/wp-content/uploads/2022/07/libftd2xx-x86_64-1.4.27.tgz
    tar xf libftd2xx-x86_64-1.4.27.tgz -C D2XX
    rm libftd2xx-x86_64-1.4.27.tgz
fi

######## Cambios en src/gcconfig.pri

if [ "${1,,}" = "debug" ]; then
    sed -i '/CONFIG += debug/ d' src/gcconfig.pri
    sed -i '/CONFIG += release/ d' src/gcconfig.pri
    sed -i '/-O3/ d' src/gcconfig.pri
    echo CONFIG += debug static >> src/gcconfig.pri
fi

sed -i '/^VLC_INSTALL/ s/^/#/' src/gcconfig.pri
sed -i '/^VLC_LIBS/ s/^/#/' src/gcconfig.pri
sed -i '/^DEFINES += GC_VIDEO_VLC/ s/^/#/' src/gcconfig.pri
sed -i "s|#\(DEFINES += GC_VIDEO_QT6.*\)|\1|" src/gcconfig.pri
sed -i "s|#\(DEFINES += GC_WANT_ROBOT*\)|\1|" src/gcconfig.pri

######## Cambios en scripts/script.sh

# El make usa tantos procesos como procesadores físicos
sed -i "s/-j4/-j$(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)/" ./scripts/script.sh

######## Cambios en src/Resources/linux/MakeAppImageQt6.sh

sed -i '
/^linuxdeployqt/ {
    /geoservices/ ! {
        s/$/ -extra-plugins=geoservices/
    }
}
' src/Resources/linux/MakeAppImageQt6.sh


sed -i 's/git log -1 >> GCversionLinuxQt6.txt/git merge-base HEAD  goldencheetah\/master |xargs git log -1>>GCversionLinuxQt6.txt/' src/Resources/linux/MakeAppImageQt6.sh
