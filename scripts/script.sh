#!/bin/bash
set -ev

# Substituye al deprecado travis/linux/script.sh (#84b26c597)
# Replica lo que se hace en appveyor.yml

qmake build.pro -r QMAKE_CXXFLAGS_WARN_ON+=\
	"-Wno-unused-private-field -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-deprecated-register -Wno-nullability-completeness -Wno-sign-compare -Wno-inconsistent-missing-override" \
	QMAKE_CFLAGS_WARN_ON+="-Wno-deprecated-declarations -Wno-sign-compare"
if test ! -f qwt/lib/libqwt.a; then make sub-qwt; fi
make -j4 sub-src
