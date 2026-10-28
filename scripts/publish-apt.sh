#!/bin/sh
# Add a .deb to the signed APT repository in MinIO and regenerate its index.
# Usage: publish-apt.sh <stable|dev> <file.deb>
# Expects an mc alias "ilvo-minio" and the signing key in the gpg keyring.
set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <stable|dev> <file.deb>" >&2
    exit 1
fi
suite=$1
deb=$2
remote=ilvo-minio/tv115-ilvo-robotics/apt
keep_dev=20

repo=$(mktemp -d)
trap 'rm -rf "$repo"' EXIT
mkdir -p "$repo/pool/$suite" "$repo/dists/$suite/main/binary-amd64"
mc mirror --quiet "$remote/pool/$suite" "$repo/pool/$suite" 2>/dev/null || true
cp "$deb" "$repo/pool/$suite/"

# Dev builds pile up, keep only the newest ones
if [ "$suite" = "dev" ]; then
    ls "$repo/pool/$suite"/*.deb | sort -V | head -n -"$keep_dev" | xargs -r rm -f
fi

cd "$repo"
apt-ftparchive packages "pool/$suite" > "dists/$suite/main/binary-amd64/Packages"
gzip -9kf "dists/$suite/main/binary-amd64/Packages"
apt-ftparchive \
    -o APT::FTPArchive::Release::Origin=ARTOF \
    -o APT::FTPArchive::Release::Suite="$suite" \
    -o APT::FTPArchive::Release::Codename="$suite" \
    -o APT::FTPArchive::Release::Components=main \
    -o APT::FTPArchive::Release::Architectures=amd64 \
    release "dists/$suite" > "dists/$suite/Release"
gpg --batch --yes --clearsign -o "dists/$suite/InRelease" "dists/$suite/Release"
gpg --batch --yes -abs -o "dists/$suite/Release.gpg" "dists/$suite/Release"
gpg --armor --export > artof-archive-keyring.asc

mc mirror --quiet --overwrite --remove "pool/$suite" "$remote/pool/$suite"
mc mirror --quiet --overwrite --remove "dists/$suite" "$remote/dists/$suite"
mc cp --quiet artof-archive-keyring.asc "$remote/artof-archive-keyring.asc"
