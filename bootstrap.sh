#!/bin/sh
# Set up a robot for the ARTOF software, once. Updates afterwards: artof-update
#   curl -fsSL https://minio.ilvo.be:9000/tv115-ilvo-robotics/bootstrap.sh | sudo sh
#   ... | sudo sh -s -- dev     # follow the development builds instead
set -e

suite=${1:-stable}
repo=https://minio.ilvo.be:9000/tv115-ilvo-robotics/apt

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this script as root (sudo)." >&2
    exit 1
fi
case "$suite" in
    stable|dev) ;;
    *) echo "Unknown channel '$suite', use 'stable' or 'dev'." >&2; exit 1 ;;
esac

# A source left by an earlier, failed run would break apt-get update
rm -f /etc/apt/sources.list.d/artof.sources

echo "Installing prerequisites"
apt-get update -qq
apt-get install -y -qq ca-certificates curl gpg
if ! command -v docker >/dev/null 2>&1; then
    apt-get install -y -qq docker.io
fi
systemctl enable --now docker

echo "Adding the ARTOF package repository ($suite)"
if ! curl -fsL -o /dev/null "$repo/dists/$suite/InRelease"; then
    echo "The '$suite' channel has no published packages yet." >&2
    [ "$suite" = "stable" ] && echo "Use the dev channel instead: ... | sudo sh -s -- dev" >&2
    exit 1
fi
install -d -m 755 /etc/apt/keyrings
curl -fsSL "$repo/artof-archive-keyring.asc" | gpg --dearmor --yes -o /etc/apt/keyrings/artof.gpg
cat > /etc/apt/sources.list.d/artof.sources <<EOF
Types: deb
URIs: $repo
Suites: $suite
Components: main
Signed-By: /etc/apt/keyrings/artof.gpg
EOF

echo "Installing artof-core"
apt-get update -qq
apt-get install -y artof-core

echo "Done. Update later with: artof-update"
