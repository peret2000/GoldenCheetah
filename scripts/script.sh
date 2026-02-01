#!/bin/bash
set -ev

# Substituye al deprecado travis/linux/script.sh (#84b26c597)
# Replica lo que se hace en appveyor.yml

# Make usa tantos procesos como procesadores físicos, salvo que se especifique
# otra cosa con $NUMMAKETHREADS
if [[ -n "$NUMMAKETHREADS" && "$NUMMAKETHREADS" -gt 0 ]]; then
    THREADS_VAL=$NUMMAKETHREADS
else
    THREADS_VAL=$(lscpu -p | grep -v '^#' | sort -u -t, -k 2,4 | wc -l)
fi

qmake build.pro -r QMAKE_CXXFLAGS_WARN_ON+="-Wno-unused-private-field -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-deprecated-register -Wno-nullability-completeness -Wno-sign-compare -Wno-inconsistent-missing-override"
if test ! -f qwt/lib/libqwt.a; then make -j${THREADS_VAL} sub-qwt; fi
make -j${THREADS_VAL} sub-src
