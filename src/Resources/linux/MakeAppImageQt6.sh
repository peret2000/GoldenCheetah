#!/bin/bash
set -ev

### This script should be run from GoldenCheetah src directory after build
if [ ! -x ./GoldenCheetah ]
then echo "Build GoldenCheetah and execute from distribution src"; exit 1
fi

qmake --version

echo "Checking GoldenCheetah.app can execute"
./GoldenCheetah --version

### Create AppDir and start populating
mkdir -p appdir

# Executable
cp GoldenCheetah appdir

# Desktop file
cat >appdir/GoldenCheetah.desktop <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=GoldenCheetah
Comment=Cycling Power Analysis Software.
Exec=GoldenCheetah
Icon=gc
Categories=Science;Sports;
EOF

# Icon
cp Resources/images/gc.png appdir/

### Add vlc 3
#mkdir appdir/lib
#cp -r /usr/lib/x86_64-linux-gnu/vlc appdir/lib/vlc
#sudo appdir/lib/vlc/vlc-cache-gen appdir/lib/vlc/plugins

### Deploy to appdir. linuxdeployqt must be in PATH
linuxdeployqt appdir/GoldenCheetah -verbose=2 -bundle-non-qt-libs -exclude-libs=libqsqlmysql,libqsqlpsql,libqsqlmimer,libqsqlodbc,libnss3,libnssutil3,libxcb-dri3.so.0 -unsupported-allow-new-glibc

## Depending on architecture, download the right appimagetool and python3.7 AppImage
ARCH="$(uname -m)"
case "$ARCH" in
  x86_64)
    AIFILE="appimagetool-x86_64.AppImage"
    ;;
  aarch64|arm64)
    AIFILE="appimagetool-aarch64.AppImage"
    ;;
  *)
    echo "Unsupported architecture: $ARCH"
    exit 1
    ;;
esac

PYTHON37DIR="$(dirname "$(dirname "$(command -v python3.7)")")"
export PATH="$PYTHON37DIR/bin:$PATH"
pip install --upgrade pip
pip install -q -r Python/requirements.txt
mkdir -p appdir/opt/python3.7
cp -rp $PYTHON37DIR appdir/opt/python3.7/

# Fix RPATH on QtWebEngineProcess and copy missing resources
patchelf --set-rpath '$ORIGIN/../lib' appdir/libexec/QtWebEngineProcess
cp -r `qmake -v|awk '/Qt/ { print $6 "/../resources" }' -` appdir

# Generate AppImage

wget --no-verbose "https://github.com/AppImage/appimagetool/releases/download/continuous/$AIFILE"
chmod a+x "$AIFILE"
export APPIMAGE_EXTRACT_AND_RUN=1
# It can fail in case of QEMU emulation, so we just warn and continue
if ! ./"$AIFILE" appdir GoldenCheetah.AppImage >/dev/null 2>&1; then
	echo "Warning: $AIFILE failed to create the AppImage (possible QEMU emulation); continuing."
	echo "'appdir' directory will remain, with the whole application."
	export FINAL_NAME=./appdir/GoldenCheetah
else
	rm -rf appdir

	if [ ! -x ./GoldenCheetah.AppImage ]
	then echo "AppImage not generated, check the errors"; exit 1
	fi

	echo "Renaming AppImage file to branch and build number ready for deploy"
	export FINAL_NAME=GoldenCheetah_v3.7_x64Qt6.AppImage
	mv -f GoldenCheetah.AppImage $FINAL_NAME
	ls -l $FINAL_NAME
fi

rm -f "$AIFILE"

### Generate version file with SHA
./$FINAL_NAME --version 2>GCversionLinuxQt6.txt
git log -1 >> GCversionLinuxQt6.txt
echo "SHA256 hash of $FINAL_NAME:" >> GCversionLinuxQt6.txt
shasum -a 256 $FINAL_NAME | cut -f 1 -d ' '  >> GCversionLinuxQt6.txt
cat GCversionLinuxQt6.txt
