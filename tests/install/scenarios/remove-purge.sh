# Removing the package keeps the settings and Redis; reinstalling picks them up.
publish 0.6.0
run_bootstrap
check_installed 0.6.0
check_healthy
mark_settings

for action in remove purge; do
    log "apt-get $action"
    run_logged apt-get "$action" -y artof-core
    check "$SERVICE is stopped" not systemctl is-active --quiet "$SERVICE"
    check "$SERVICE is not enabled" not systemctl is-enabled --quiet "$SERVICE"
    check "settings are kept" settings_marked
    check "Redis container is kept" docker container inspect "$REDIS_CONTAINER"
done

log "Installing again"
run_logged apt-get install -y artof-core
check_installed 0.6.0
check "settings edit is kept" settings_marked
check_healthy
