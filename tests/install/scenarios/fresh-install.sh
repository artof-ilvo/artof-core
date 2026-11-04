# A new robot: bootstrap installs Docker, the repository and the defaults.
publish 0.6.0
run_bootstrap
check_installed 0.6.0
check_healthy
check "settings equal the defaults" diff -r -x logs "$DEFAULTS" "$ILVO_PATH"
check "no backup of an empty $ILVO_PATH" test "$(backup_count)" -eq 0
check "systemd unit is valid" systemd-analyze verify "$SERVICE"
check "artof-update --list shows the installed version" sh -c 'artof-update --list | grep -q "Installed: 0.6.0"'

log "Running the bootstrap again"
before=$(settings_hash)
run_bootstrap
check_installed 0.6.0
check "exactly one ARTOF apt source" test "$(grep -rl "$REPO_URL" /etc/apt/sources.list.d | wc -l)" -eq 1
check "settings unchanged" test "$(settings_hash)" = "$before"
check_healthy
