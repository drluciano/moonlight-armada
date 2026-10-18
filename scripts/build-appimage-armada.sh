#!/bin/bash
# Packages an aarch64 Moonlight Armada AppImage for ArmadaOS (Arch-based).
#
# Usage: scripts/build-appimage-armada.sh <build dir> <output AppImage>
#
# <build dir> is a qmake build of moonlight-qt.pro; the binary is expected at
# <build dir>/app/moonlight. Works natively on arm64 or cross-compiled with the
# multiarch arm64 packages installed (set CROSS=aarch64-linux-gnu-).
#
# The host is only expected to provide core desktop libraries that every Arch
# desktop or Steam install has (glibc, Mesa/GL, X11/xcb, Wayland, glib,
# fontconfig/freetype, ALSA, dbus, systemd). Everything else is bundled.
set -euo pipefail

fail()
{
	echo "$1" 1>&2
	exit 1
}

[ $# -eq 2 ] || fail "Usage: $0 <build dir> <output AppImage>"

BUILD_DIR=$(readlink -f "$1")
OUTPUT=$(readlink -m "$2")
SOURCE_ROOT=$(readlink -f "$(dirname "$0")/..")
DEPLOY_SRC=$SOURCE_ROOT/app/deploy/linux
CROSS=${CROSS:-}
READELF=${CROSS}readelf
STRIP=${CROSS}strip
CC=${CC:-${CROSS}gcc}
APPIMAGETOOL=${APPIMAGETOOL:-appimagetool}
LIB=/usr/lib/aarch64-linux-gnu
QT=$LIB/qt6

[ -x "$BUILD_DIR/app/moonlight" ] || fail "No moonlight binary in $BUILD_DIR/app"
command -v $READELF >/dev/null || fail "Unable to find '$READELF'"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
AD=$WORK/AppDir
mkdir -p $AD/usr/bin $AD/usr/lib $AD/usr/plugins $AD/usr/qml $AD/opt/libva-fallback

cp $BUILD_DIR/app/moonlight $AD/usr/bin/moonlight
$STRIP $AD/usr/bin/moonlight

for p in platforms wayland-shell-integration wayland-decoration-client wayland-graphics-integration-client \
         xcbglintegrations egldeviceintegrations imageformats iconengines tls platforminputcontexts generic; do
  [ -d $QT/plugins/$p ] && cp -r $QT/plugins/$p $AD/usr/plugins/
done
for q in QtQuick QtQml QtCore; do
  [ -d $QT/qml/$q ] && cp -r $QT/qml/$q $AD/usr/qml/
done
rm -rf $AD/usr/qml/QtQuick/Controls/{Imagine,Universal,Fusion,FluentWinUI3,Windows,macOS,iOS}

cat > $AD/usr/bin/qt.conf <<EOF
[Paths]
Prefix=..
Plugins=plugins
Qml2Imports=qml
QmlImports=qml
EOF

# Qt dlopens libssl at runtime, so it never shows up as a dependency. It must
# match the libcrypto we bundle, or a newer host libssl fails to load against it.
cp -L $LIB/libssl.so.3 $AD/usr/lib/

# libproxy pulls in curl, gnutls and p11-kit, which load host plugins at startup.
# Ship a stub that always reports a direct connection instead.
$CC -O2 -shared -fPIC -Wl,-soname,libproxy.so.1 \
  -Wl,--version-script=$DEPLOY_SRC/armada/libproxy-stub.map \
  -o $WORK/libproxy.so.1 $DEPLOY_SRC/armada/libproxy-stub.c || fail "Unable to build libproxy stub!"

# Libraries the host must provide
EXCL='^(ld-linux|libc\.so|libm\.so|libdl|libpthread|librt\.so|libresolv|libutil|libstdc\+\+|libgcc_s|libGL|libEGL|libGLX|libGLdispatch|libOpenGL|libGLES|libgbm|libdrm|libvulkan|libwayland-|libX11|libXext|libXrender|libXfixes|libXi\.|libXrandr|libXcursor|libxcb\.|libxcb-(dri3|glx|render\.|shm|sync|xfixes|randr|shape|xkb)|libxkbcommon\.|libasound|libfontconfig|libfreetype|libexpat|libz\.so|libdbus|libudev|libsystemd|libpipewire|libharfbuzz|libglib|libgobject|libgio|libgmodule|libpcre2-8|libffi|libmount|libblkid|libcap|libgcrypt|libgpg-error|liblzma|libzstd|liblz4|libbz2|libuuid|libpng|libbrotli|libgraphite)'

declare -A seen
queue=()
missing=0
while IFS= read -r f; do queue+=("$f"); done < <(find $AD/usr -type f \( -name '*.so*' -o -name moonlight \))
while [ ${#queue[@]} -gt 0 ]; do
  f=${queue[0]}; queue=("${queue[@]:1}")
  for n in $($READELF -d "$f" 2>/dev/null | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p'); do
    [[ -n "${seen[$n]:-}" ]] && continue
    seen[$n]=1
    [[ $n =~ $EXCL ]] && continue
    if [ "$n" = libproxy.so.1 ]; then cp $WORK/libproxy.so.1 $AD/usr/lib/; continue; fi
    src=$LIB/$n
    [ -e $src ] || src=$(find $LIB /lib/aarch64-linux-gnu -name "$n" 2>/dev/null | head -1)
    if [ -z "$src" ] || [ ! -e "$src" ]; then echo "MISSING $n (needed by $(basename $f))"; missing=1; continue; fi
    cp -L $src $AD/usr/lib/$n
    queue+=("$AD/usr/lib/$n")
  done
done
[ $missing -eq 0 ] || fail "Unable to resolve all libraries!"

# Like upstream, prefer the host libva so the host's VA drivers can load
mv $AD/usr/lib/libva*.so* $AD/opt/libva-fallback/

cp $DEPLOY_SRC/armada/AppRun $AD/AppRun
chmod +x $AD/AppRun
sed -e 's/^Name=.*/Name=Moonlight Armada/' $DEPLOY_SRC/com.moonlight_stream.Moonlight.desktop > $AD/moonlight.desktop
cp $SOURCE_ROOT/app/res/moonlight.svg $AD/moonlight.svg
ln -s moonlight.svg $AD/.DirIcon

echo "Host libraries required:"
for f in $(find $AD/usr -type f \( -name '*.so*' -o -name moonlight \)); do
  $READELF -d $f 2>/dev/null | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p'
done | sort -u | while read n; do [ -e $AD/usr/lib/$n ] || [ -e $AD/opt/libva-fallback/$n ] || echo "  $n"; done

mkdir -p "$(dirname "$OUTPUT")"
ARCH=aarch64 $APPIMAGETOOL ${RUNTIME_FILE:+--runtime-file $RUNTIME_FILE} $AD "$OUTPUT" || fail "appimagetool failed!"
echo "Created $OUTPUT"
