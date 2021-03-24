#!/bin/bash

export PATH=/opt/qt514/bin:$PATH
export LD_LIBRARY_PATH=/opt/qt514/lib/x86_64-linux-gnu:/opt/qt514/lib:$LD_LIBRARY_PATH

#export QT_OPENGL=software

`dirname $0`/../src/GoldenCheetah $1


