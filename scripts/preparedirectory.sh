#!/bin/bash

# IMPORTANTE: Ejecutar desde el directorio del repositorio: por ejemplo:
# ./scripts/preparedirectory.sh


# Prepara el directorio pasado como argumento para compilar por primera vez
# Ejecuta travis/linux/before_script.sh (debe estar la variable de entorno $GC_STRAVA_CLIENT_SECRET)
# modifica gcconfig.pri


# Aquí se debe poner la variables de entorno $GC_STRAVA_CLIENT_SECRET (o existir ya) si se quiere compilar con ella

travis/linux/before_script.sh

sed -i '/GC_VERSION/ d' src/gcconfig.pri
echo DEFINES += GC_VERSION=\"\\\\\\\"\\\\\(Debug\\ `git log -1  goldencheetah/master | sed -n 's/^commit *//p' | cut -c -7`\\\\\)\\\\\\\"\"  >> src/gcconfig.pri

sed -i '/CONFIG += debug/ d' src/gcconfig.pri
sed -i '/CONFIG += release/ d' src/gcconfig.pri
sed -i '/-O3/ d' src/gcconfig.pri

echo CONFIG += debug static >> src/gcconfig.pri


