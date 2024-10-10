#!/bin/sh
BUILDDIR=./abaddon-0.2.1

if [ ! -d ${BUILDDIR} ]; then
    echo "Directory '${BUILDDIR}' does not exist."
    exit
fi

rm -r ${BUILDDIR}/include/
rm -r ${BUILDDIR}/lib/

mkdir -p ${BUILDDIR}/etc/ssl/certs/
cp /mingw64/etc/ssl/certs/ca-bundle.crt ${BUILDDIR}/etc/ssl/certs/

mkdir -p ${BUILDDIR}/lib/
cp -r /mingw64/lib/gdk-pixbuf-2.0 ${BUILDDIR}/lib/

mkdir -p ${BUILDDIR}/share/glib-2.0/schemas/
cp /mingw64/share/glib-2.0/schemas/gschemas.compiled ${BUILDDIR}/share/glib-2.0/schemas/

cat "../ci/msys-deps.txt" | sed 's/\r$//' | xargs -I % cp /mingw64% ${BUILDDIR}/bin/ || :
cp /usr/bin/msys-ffi-8.dll ${BUILDDIR}/bin/libffi-8.dll

mkdir -p ${BUILDDIR}/share/themes/
wget -nc https://github.com/rtlewis1/GTK/archive/refs/heads/Material-Black-Colors-Desktop.zip
unzip -q -o Material-Black-Colors-Desktop.zip 'GTK-Material-Black-Colors-Desktop/Material-Black-Cherry/**/*'
mv ./GTK-Material-Black-Colors-Desktop/Material-Black-Cherry ${BUILDDIR}/share/themes/
cp -r ../ci/tree/. ${BUILDDIR}/

mkdir -p ${BUILDDIR}/share/icons/Adwaita/{16x16,24x24,32x32,48x48,64x64,96x96,scalable}/{actions,devices,status,places}
cp ../ci/gtk-for-windows/gtk-nsis-pack/share/icons/Adwaita/index.theme ${BUILDDIR}/share/icons/Adwaita/
for res in 16x16 24x24 32x32 48x48 64x64 96x96; do \
	cat "../ci/used-icons.txt" | sed 's/\r$//' | \
	xargs -I % cp ../ci/gtk-for-windows/gtk-nsis-pack/share/icons/Adwaita/${res}/%.symbolic.png \
	${BUILDDIR}/share/icons/Adwaita/${res}/%.symbolic.png || : \
; done
cat "../ci/used-icons.txt" | sed 's/\r$//' | \
	xargs -I % cp ../ci/gtk-for-windows/gtk-nsis-pack/share/icons/Adwaita/scalable/%.svg \
	${BUILDDIR}/share/icons/Adwaita/scalable/%.svg || :
cd ${BUILDDIR}/share/icons/Adwaita/
gtk-update-icon-cache .

