#!/bin/bash

export PATH=/opt/qt515/bin:$PATH
export LD_LIBRARY_PATH=/opt/qt515/lib/x86_64-linux-gnu:/opt/qt515/lib:$LD_LIBRARY_PATH

#export QT_OPENGL=software

`dirname $0`/../src/GoldenCheetah $1


