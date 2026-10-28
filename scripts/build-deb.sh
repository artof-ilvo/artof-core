#!/bin/sh
# Build the artof-core .deb from a package tree with the binaries installed.
# Usage: build-deb.sh <package-dir> <version> <output.deb>
set -e

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <package-dir> <version> <output.deb>" >&2
    exit 1
fi
pkg=$(realpath "$1")
version=$2
output=$3

# dpkg-shlibdeps needs a debian/control to run in
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir "$work/debian"
printf 'Source: artof-core\n\nPackage: artof-core\nArchitecture: amd64\n' > "$work/debian/control"
shlibs=$(cd "$work" && dpkg-shlibdeps -O --ignore-missing-info -l"$pkg/usr/lib/ilvo" \
    "$pkg"/usr/bin/ilvo-* | sed -n 's/^shlibs:Depends=//p')
if [ -z "$shlibs" ]; then
    echo "Could not determine the shared library dependencies" >&2
    exit 1
fi

stage="$work/artof-core"
cp -a "$pkg" "$stage"
sed -i -e "s/@VERSION@/$version/" -e "s/@SHLIBS_DEPENDS@/$shlibs/" "$stage/DEBIAN/control"
chmod -R go-w "$stage"
chmod 755 "$stage"/DEBIAN/preinst "$stage"/DEBIAN/postinst "$stage"/DEBIAN/prerm "$stage"/DEBIAN/postrm \
    "$stage"/usr/bin/* "$stage"/usr/lib/ilvo/libsnap7.so

dpkg-deb --root-owner-group --build "$stage" "$output"
