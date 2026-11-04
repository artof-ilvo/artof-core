#!/bin/sh
# Run one installation scenario against a local, signed APT repository.
#   sudo env ARTOF_INSTALL_TEST=1 tests/install/run.sh <artof-core.deb> <scenario>
# Scenarios live in tests/install/scenarios/. The .deb is repacked as 0.5.0 and
# 0.6.0, and a stand-in for the legacy 0.0.1 package is built from it.
# This purges Docker and writes /var/lib/ilvo and the apt sources, so run it
# only on a disposable VM or CI runner.
set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <artof-core.deb> <scenario>" >&2
    exit 1
fi
if [ "$(id -u)" -ne 0 ]; then
    echo "Run this script as root (sudo)." >&2
    exit 1
fi
if [ "${ARTOF_INSTALL_TEST:-}" != 1 ]; then
    echo "This changes the whole machine. Set ARTOF_INSTALL_TEST=1 on a disposable VM." >&2
    exit 1
fi

deb=$(realpath "$1")
here=$(cd "$(dirname "$0")" && pwd)
scenario="$here/scenarios/$2.sh"
SRC=$(cd "$here/../.." && pwd)
[ -f "$scenario" ] || { echo "Unknown scenario '$2'" >&2; exit 1; }

. "$here/lib.sh"
# shellcheck disable=SC2154  # status is set inside the trap
trap 'status=$?; [ "$status" -eq 0 ] || diagnostics' EXIT

rm -rf "$WORK"
mkdir -p "$WORK"
log "Preparing packages and the local repository"
prepare_debs "$deb"
start_repo
purge_docker

log "Scenario: $2"
# shellcheck source=/dev/null
. "$scenario"
log "Scenario $2 passed"
