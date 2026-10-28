#!/bin/sh
# Add a .deb to the signed APT repository in MinIO and regenerate its index.
# Usage: publish-apt.sh <stable|dev> <file.deb>
# Expects the signing key in the gpg keyring and AWS_ACCESS_KEY_ID,
# AWS_SECRET_ACCESS_KEY and MINIO_ENDPOINT in the environment.
set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <stable|dev> <file.deb>" >&2
    exit 1
fi
suite=$1
deb=$2
remote=s3://tv115-ilvo-robotics/apt
keep_dev=20

# MinIO rejects the checksum headers newer AWS CLI versions send by default
export AWS_DEFAULT_REGION=${AWS_DEFAULT_REGION:-us-east-1}
export AWS_REQUEST_CHECKSUM_CALCULATION=when_required
export AWS_RESPONSE_CHECKSUM_VALIDATION=when_required
s3() { aws --endpoint-url "$MINIO_ENDPOINT" s3 --only-show-errors "$@"; }

repo=$(mktemp -d)
trap 'rm -rf "$repo"' EXIT
mkdir -p "$repo/pool/$suite" "$repo/dists/$suite/main/binary-amd64"
s3 sync "$remote/pool/$suite" "$repo/pool/$suite"
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

s3 sync --delete "pool/$suite" "$remote/pool/$suite"
s3 sync --delete "dists/$suite" "$remote/dists/$suite"
s3 cp artof-archive-keyring.asc "$remote/artof-archive-keyring.asc"
