#!/usr/bin/bash
set -e

# Configuration
APP_NAME="abaddon"
APP_VERSION=$(git describe --tags --exact-match >/dev/null 2>&1 && git describe --tags --abbrev=0 | sed 's/^v//' || echo "git-$(git rev-parse --short=16 HEAD)")
ARCH="$(uname -m)"
BUILD_DIR="build"
APPDIR="${BUILD_DIR}/${APP_NAME}.AppDir"

echo "Building ${APP_NAME} AppImage for ${ARCH}..."

# Clean previous builds
rm -rf "$APPDIR"
rm -rf "$BUILD_DIR"
mkdir -p "${APPDIR}"

# Build the application
echo "Building application..."
(
  cd build && \
  cmake .. && \
  make
)

# Create AppDir structure
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib"
mkdir -p "${APPDIR}/usr/local/share/${APP_NAME}"
mkdir -p "${APPDIR}/usr/share/metainfo/"

# Copy executable
cp "${BUILD_DIR}/${APP_NAME}" "${APPDIR}/usr/bin/"

# Desktop file
if [ -f "res/desktop/io.github.uowuo.abaddon.desktop" ]; then
    cp "res/desktop/io.github.uowuo.abaddon.desktop" "${APPDIR}/${APP_NAME}.desktop"
    sed -i 's/^Icon=.*/Icon=abaddon/' "${APPDIR}/${APP_NAME}.desktop"
else
    echo "Warning: ${APP_NAME}.desktop not found, creating basic one..."
    cat > "${APPDIR}/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Type=Application
Name=Abaddon
Exec=${APP_NAME}
Icon=${APP_NAME}
Categories=Network;Chat;
Terminal=false
EOF
fi

# Copy icon
if [ -f "res/desktop/icon.svg" ]; then
    cp "res/desktop/icon.svg" "${APPDIR}/${APP_NAME}.svg"
elif [ -f "res/desktop/icon.png" ]; then
    cp "res/desktop/icon.png" "${APPDIR}/${APP_NAME}.png"
fi

if [ -f "res/desktop/io.github.uowuo.abaddon.metainfo.xml" ]; then
    cp "res/desktop/io.github.uowuo.abaddon.metainfo.xml" "${APPDIR}/usr/share/metainfo/"
fi

# Copy resources
if [ -d "res" ]; then
    cp -r "res/." "${APPDIR}/usr/local/share/${APP_NAME}/"
fi

# AppRun
cat > "$APPDIR/AppRun" <<'APPRUN_EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}

# Only add our lib directory at the END to prefer system libraries
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${HERE}/usr/lib"
export PATH="${HERE}/usr/bin:${PATH}"
export XDG_DATA_DIRS="${HERE}/usr/share:${HERE}/usr/local/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
# export GSETTINGS_SCHEMA_DIR="${HERE}/usr/share/librediscord/assets"

# GTK theme and settings
export GTK_THEME=${GTK_THEME:-Adwaita}

# Prevent loading incompatible GIO modules by using system's modules
# This avoids symbol version mismatches with system GLib
unset GIO_MODULE_DIR
export GIO_EXTRA_MODULES=/usr/lib/x86_64-linux-gnu/gio/modules

# Use system GTK modules
unset GTK_PATH
unset GTK_IM_MODULE_FILE

# Execute the application
exec "$HERE/usr/bin/abaddon" "$@"
APPRUN_EOF

chmod +x "$APPDIR/AppRun"

# Download linuxdeploy if needed
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    wget -O linuxdeploy-x86_64.AppImage \
        https://github.com/linuxdeploy/linuxdeploy/releases/latest/download/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

# Build AppImage
echo "Creating AppImage..."
# VERSION="$APP_VERSION" ARCH="$ARCH" \
# ./squashfs-root/AppRun \
VERSION="$APP_VERSION" ARCH="$ARCH" ./linuxdeploy-x86_64.AppImage \
    --appdir "$APPDIR" \
    --desktop-file "$APPDIR/$APP_NAME.desktop" \
    --icon-file "$APPDIR/$APP_NAME.svg" \
    --executable "$APPDIR/usr/bin/$APP_NAME" \
    --output appimage

echo "AppImage created: ${APP_NAME}-${APP_VERSION}-${ARCH}.AppImage"
echo ""
echo "You can now run: ./${APP_NAME}-${APP_VERSION}-${ARCH}.AppImage"
