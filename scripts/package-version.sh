#!/bin/sh
# Print the Debian package version derived from git:
#   on tag v1.2.0            -> 1.2.0
#   3 commits after v1.2.0   -> 1.2.0+3.g1a2b3c4   (sorts after 1.2.0)
#   no release tag yet       -> 0.1.0~54.g1a2b3c4  (sorts after the legacy 0.0.1)
set -e

if desc=$(git describe --tags --match 'v[0-9]*' 2>/dev/null); then
    echo "$desc" | sed -E 's/^v//; s/-([0-9]+)-g([0-9a-f]+)$/+\1.g\2/'
else
    echo "0.1.0~$(git rev-list --count HEAD).g$(git rev-parse --short HEAD)"
fi
