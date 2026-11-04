# Robots on 0.0.1 had the settings as package files, which dpkg deletes.
publish 0.0.1
run_bootstrap
check_installed 0.0.1
mark_settings
# The old install script wrote its own unit to /etc
cat > "/etc/systemd/system/$SERVICE" <<UNIT
[Service]
ExecStart=ilvo-system-manager
UNIT
systemctl daemon-reload

log "Upgrading from the legacy package"
publish 0.0.1 0.6.0
run_logged apt-get update -qq
run_logged apt-get install -y artof-core
check_installed 0.6.0
check "settings edit survived the upgrade" settings_marked
check "legacy copy is cleaned up" not test -e /var/lib/ilvo.legacy-upgrade
check "legacy unit in /etc is removed" not test -e "/etc/systemd/system/$SERVICE"
check "the packaged unit is used" sh -c "systemctl show -p FragmentPath $SERVICE | grep -q '/lib/systemd/system/'"
check "a backup was made" test "$(backup_count)" -eq 1
check_healthy
