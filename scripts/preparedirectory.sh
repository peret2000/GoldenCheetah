#!/bin/bash

# IMPORTANTE: Ejecutar desde el directorio del repositorio: por ejemplo:
# ./scripts/preparedirectory.sh


# Prepara el directorio para compilar por primera vez

# Si se ejecuta el script diario (daily_script.sh), ya prepara el directorio, pues llama a este script.
# Si no, es necesario ejecutar este script, para preparar una compilación, tanto de QtCreator como manual

# Ejecuta travis/linux/before_script.sh (debe estar la variable de entorno $GC_STRAVA_CLIENT_SECRET)
# modifica gcconfig.pri

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

# Aquí se debe poner la variables de entorno $GC_STRAVA_CLIENT_SECRET (o existir ya) si se quiere compilar con ella

travis/linux/before_script.sh || { ERR=$?; exit $ERR; }

# Extracted from travis/linux/before_install.sh : Downloads necessary header file if it was not downloaded yet
# D2XX - refresh cache if folder is empty
mkdir -p D2XX
if [ -z "$(ls -A D2XX)" ]; then
    wget --no-verbose https://ftdichip.com/wp-content/uploads/2022/07/libftd2xx-x86_64-1.4.27.tgz
    tar xf libftd2xx-x86_64-1.4.27.tgz -C D2XX
    rm libftd2xx-x86_64-1.4.27.tgz
fi



sed -i '/GC_VERSION/ d' src/gcconfig.pri
echo DEFINES += GC_VERSION=\"\\\\\\\"\\\\\(Debug\\ `git log -1  goldencheetah/master | sed -n 's/^commit *//p' | cut -c -7`\\\\\)\\\\\\\"\"  >> src/gcconfig.pri

## Se genera la version release, ya que no es útil para depurar
# sed -i '/CONFIG += debug/ d' src/gcconfig.pri
# sed -i '/CONFIG += release/ d' src/gcconfig.pri
# sed -i '/-O3/ d' src/gcconfig.pri
# echo CONFIG += debug static >> src/gcconfig.pri

# El make usa tantos procesos como procesadores físicos
sed -i "s/-j4/-j$(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)/" travis/linux/script.sh
