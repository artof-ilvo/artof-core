# Helpers for the installation scenarios, sourced by run.sh.
# shellcheck disable=SC2034  # the constants are used by the scenarios

ILVO_PATH=/var/lib/ilvo
BACKUP_DIR=/var/lib/ilvo-backups
DEFAULTS=/usr/share/artof-core/defaults
SERVICE=ilvo-robotics.service
REDIS_CONTAINER=redis-stack-server
REDIS_IMAGE=redis/redis-stack-server:7.4.0-v8
SYSTEM_IMAGE=axelwillekens/artof-system
REPO_PORT=8765
REPO_URL=http://localhost:$REPO_PORT
WORK=/tmp/artof-install-test
export GNUPGHOME="$WORK/gnupg"

log() { printf '\n==> %s\n' "$*"; }
fail() { echo "FAIL: $*" >&2; exit 1; }
not() { ! "$@"; }

# check <description> <command...>
check() {
    desc=$1; shift
    if "$@"; then echo "ok: $desc"; else fail "$desc"; fi
}

# wait_until <seconds> <command...>
wait_until() {
    timeout=$1; shift
    for _ in $(seq "$timeout"); do
        "$@" >/dev/null 2>&1 && return 0
        sleep 1
    done
    return 1
}

# run_logged <command...>: run, keep the output in $WORK/last.log and show it
run_logged() {
    if "$@" >"$WORK/last.log" 2>&1; then
        cat "$WORK/last.log"
    else
        cat "$WORK/last.log"
        fail "command failed: $*"
    fi
}
last_log_contains() { grep -q -- "$1" "$WORK/last.log"; }

# Package variants ---------------------------------------------------------

# repack <in.deb> <version> <out.deb>
repack() {
    tmp=$(mktemp -d)
    dpkg-deb -R "$1" "$tmp/pkg"
    sed -i "s/^Version: .*/Version: $2/" "$tmp/pkg/DEBIAN/control"
    dpkg-deb --root-owner-group -b "$tmp/pkg" "$3" >/dev/null
    rm -rf "$tmp"
}

# Packages up to 0.0.1 shipped the settings as package files in /var/lib/ilvo
build_legacy() {
    tmp=$(mktemp -d)
    dpkg-deb -x "$1" "$tmp/new"
    mkdir -p "$tmp/pkg/DEBIAN" "$tmp/pkg/var/lib"
    cp -r "$tmp/new$DEFAULTS" "$tmp/pkg$ILVO_PATH"
    cat > "$tmp/pkg/DEBIAN/control" <<EOF
Package: artof-core
Version: 0.0.1
Maintainer: ARTOF install test <noreply@ilvo.vlaanderen.be>
Architecture: amd64
Description: Stand-in for the legacy package that shipped the settings
EOF
    dpkg-deb --root-owner-group -b "$tmp/pkg" "$2" >/dev/null
    rm -rf "$tmp"
}

prepare_debs() {
    mkdir -p "$WORK/debs"
    repack "$1" 0.5.0 "$WORK/debs/artof-core_0.5.0_amd64.deb"
    repack "$1" 0.6.0 "$WORK/debs/artof-core_0.6.0_amd64.deb"
    build_legacy "$1" "$WORK/debs/artof-core_0.0.1_amd64.deb"
}

# Local APT repository -----------------------------------------------------

start_repo() {
    mkdir -p "$WORK/repo" "$GNUPGHOME"
    chmod 700 "$GNUPGHOME"
    gpg --batch --pinentry-mode loopback --passphrase '' \
        --quick-gen-key 'ARTOF install test' rsa3072 sign never 2>/dev/null
    nohup python3 -m http.server "$REPO_PORT" -d "$WORK/repo" >"$WORK/http.log" 2>&1 &
    wait_until 10 curl -fs "$REPO_URL/" || fail "local APT repository did not start"
}

# publish <version...>: the repository holds exactly these versions afterwards
publish() {
    repo=$WORK/repo
    rm -rf "$repo/pool" "$repo/dists"
    mkdir -p "$repo/pool/dev" "$repo/dists/dev/main/binary-amd64"
    for v in "$@"; do cp "$WORK/debs/artof-core_${v}_amd64.deb" "$repo/pool/dev/"; done
    (
        cd "$repo" || exit 1
        apt-ftparchive packages pool/dev > dists/dev/main/binary-amd64/Packages
        gzip -9kf dists/dev/main/binary-amd64/Packages
        apt-ftparchive \
            -o APT::FTPArchive::Release::Origin=ARTOF \
            -o APT::FTPArchive::Release::Suite=dev \
            -o APT::FTPArchive::Release::Codename=dev \
            -o APT::FTPArchive::Release::Components=main \
            -o APT::FTPArchive::Release::Architectures=amd64 \
            release dists/dev > "$WORK/Release"
        mv "$WORK/Release" dists/dev/Release
        gpg --batch --yes --clearsign -o dists/dev/InRelease dists/dev/Release
        gpg --batch --yes -abs -o dists/dev/Release.gpg dists/dev/Release
        gpg --armor --export > artof-archive-keyring.asc
    )
    echo "published: $*"
}

# Machine ------------------------------------------------------------------

# Remove the Docker that CI images ship, so the bootstrap installs it like on a
# fresh robot
purge_docker() {
    pkgs=$(dpkg-query -W -f '${Package} ${db:Status-Status}\n' 2>/dev/null \
        | awk '$2 == "installed" {print $1}' \
        | grep -E '^(docker-ce|docker-ce-cli|docker-ce-rootless-extras|docker-buildx-plugin|docker-compose-plugin|containerd\.io|docker\.io|containerd|runc|moby-.*)$' || true)
    if [ -n "$pkgs" ]; then
        # shellcheck disable=SC2086
        apt-get purge -y -qq $pkgs >/dev/null
    fi
}

run_bootstrap() {
    run_logged env ARTOF_REPO="$REPO_URL" sh "$SRC/bootstrap.sh" dev
}

reinstall() { run_logged apt-get install --reinstall -y artof-core; }

installed_version() { dpkg-query -W -f '${Version}' artof-core 2>/dev/null; }
package_status() { dpkg-query -W -f '${Status}' artof-core 2>/dev/null; }

# Checks -------------------------------------------------------------------

check_installed() {
    check "installed version is $1" test "$(installed_version)" = "$1"
    check "package is fully configured" test "$(package_status)" = "install ok installed"
}

redis_initialized() { [ -n "$(redis-cli JSON.GET system 2>/dev/null)" ]; }
redis_image_pinned() {
    [ "$(docker inspect -f '{{.Config.Image}}' "$REDIS_CONTAINER" 2>/dev/null)" = "$REDIS_IMAGE" ]
}

# Settings belong to uid 1000 when it exists, otherwise to root. The service
# itself runs as root and writes logs/, so that is left out.
settings_owned() {
    if getent passwd 1000 >/dev/null; then uid=1000; else uid=0; fi
    [ -z "$(find "$ILVO_PATH" -path "$ILVO_PATH/logs" -prune -o ! -uid "$uid" -print)" ]
}

check_healthy() {
    check "package is fully configured" test "$(package_status)" = "install ok installed"
    check "Redis container runs the pinned image" redis_image_pinned
    check "Redis answers" wait_until 30 redis-cli ping
    check "$SERVICE is enabled" systemctl is-enabled --quiet "$SERVICE"
    check "$SERVICE is active" wait_until 60 systemctl is-active --quiet "$SERVICE"
    check "system manager initialized Redis" wait_until 60 redis_initialized
    check "settings are owned by the robot user" settings_owned
    check "every default exists in $ILVO_PATH" defaults_present
}

defaults_present() {
    for default in "$DEFAULTS"/*; do
        [ -e "$ILVO_PATH/$(basename "$default")" ] || return 1
    done
}

# Hash of the robot settings, without logs
settings_hash() {
    (cd "$ILVO_PATH" && find . -path ./logs -prune -o -type f -exec sha256sum {} + | sort | sha256sum)
}

mark_settings() {
    python3 - "$ILVO_PATH/settings.json" <<'EOF'
import json, sys
with open(sys.argv[1]) as f:
    settings = json.load(f)
settings["install_test_marker"] = True
with open(sys.argv[1], "w") as f:
    json.dump(settings, f, indent=4)
EOF
}
settings_marked() { grep -q '"install_test_marker"' "$ILVO_PATH/settings.json"; }

backup_count() { find "$BACKUP_DIR" -name 'ilvo-*.tar.gz' 2>/dev/null | wc -l; }
newest_backup() { ls -1t "$BACKUP_DIR"/ilvo-*.tar.gz | head -n 1; }
newest_backup_from() { newest_backup | grep -q -- "-$1.tar.gz\$"; }
newest_backup_marked() { tar -xOzf "$(newest_backup)" ilvo/settings.json | grep -q install_test_marker; }
newest_backup_has_logs() { tar -tzf "$(newest_backup)" | grep -q '^ilvo/logs/'; }

diagnostics() {
    echo "---- diagnostics ----" >&2
    echo "package: $(installed_version) [$(package_status)]" >&2
    systemctl status "$SERVICE" --no-pager >&2 || true
    journalctl -u "$SERVICE" -n 80 --no-pager >&2 || true
    docker ps -a >&2 || true
    ls -la "$ILVO_PATH" "$BACKUP_DIR" >&2 || true
}
