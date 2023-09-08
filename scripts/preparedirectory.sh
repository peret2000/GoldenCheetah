#!/bin/bash

# IMPORTANTE: Ejecutar desde el directorio del repositorio: por ejemplo:
# ./scripts/preparedirectory.sh


# Prepara el directorio pasado como argumento para compilar por primera vez

# Si se ejecuta el script diario (daily_script.sh), ya prepara el directorio, para compilación release.
# Si no, es conveniente ejecutar este script, para preparar una compilación, tanto de QtCreator como manual

# Ejecuta travis/linux/before_script.sh (debe estar la variable de entorno $GC_STRAVA_CLIENT_SECRET)
# modifica gcconfig.pri

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi

# Aquí se debe poner la variables de entorno $GC_STRAVA_CLIENT_SECRET (o existir ya) si se quiere compilar con ella

travis/linux/before_script.sh

sed -i '/GC_VERSION/ d' src/gcconfig.pri
echo DEFINES += GC_VERSION=\"\\\\\\\"\\\\\(Debug\\ `git log -1  goldencheetah/master | sed -n 's/^commit *//p' | cut -c -7`\\\\\)\\\\\\\"\"  >> src/gcconfig.pri

sed -i '/CONFIG += debug/ d' src/gcconfig.pri
sed -i '/CONFIG += release/ d' src/gcconfig.pri
sed -i '/-O3/ d' src/gcconfig.pri

echo CONFIG += debug static >> src/gcconfig.pri

# El make usa tantos procesos como procesadores físicos
sed -i "s/-j4/-j$(lscpu -p | egrep -v '^#' | sort -u -t, -k 2,4 | wc -l)/" travis/linux/script.sh
